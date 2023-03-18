/* VLDP [https://www.cs.utah.edu/~rajeev/pubs/micro15m.pdf] */

#include <bits/stdc++.h>

using namespace std;

/** End Of Cache Framework **/

class ShiftRegister {
  public:
    /* the maximum total capacity of this shift register is 64 bits */
    ShiftRegister(unsigned size = 4) : size(size), width(64 / size) {}

    void insert(int x) {
        x &= (1 << this->width) - 1;
        this->reg = (this->reg << this->width) | x;
    }

    /*
     * @return Returns raw buffer data in range [le, ri).
     * Note that more recent data have a smaller index.
     */
    uint64_t get_code(unsigned le, unsigned ri) {
        assert(0 <= le && le < this->size);
        assert(le < ri && ri <= this->size);
        uint64_t mask = (1ull << (this->width * (ri - le))) - 1ull;
        return (this->reg >> (le * this->width)) & mask;
    }

    /*
     * @return Returns integer value of data at specified index.
     */
    int get_value(int i) {
        int x = this->get_code(i, i + 1);
        /* sign extend */
        int d = 32 - this->width;
        return (x << d) >> d;
    }

  private:
    unsigned size;
    unsigned width;
    uint64_t reg = 0;
};

class SaturatingCounter {
  public:
    SaturatingCounter(int size = 2) : size(size), max((1 << size) - 1) {}

    int inc() {
        this->cnt += 1;
        if (this->cnt > this->max)
            this->cnt = this->max;
        return this->cnt;
    }

    int dec() {
        this->cnt -= 1;
        if (this->cnt < 0)
            this->cnt = 0;
        return this->cnt;
    }

    int get_cnt() { return this->cnt; }

  private:
    int size, max, cnt = 0;
};

class DeltaHistoryBufferData {
  public:
    uint64_t last_addr;
    ShiftRegister deltas;
    int last_predictor;
    int num_times_used;
    /* Each entry in the DHB contains the following data for a tracked physical page: (i) page number, (ii) page offset
     * of the last address accessed in this page, (iii) sequence of up to 4 recently observed deltas, (iv) the DPT level
     * used for the latest delta prediction, (v) the number of times this page has been used, and (vi) sequence of up to
     * 4 recently prefetched offsets. */
};

class DeltaHistoryBuffer : public NMRUFullyAssociativeCache<DeltaHistoryBufferData> {
    typedef NMRUFullyAssociativeCache<DeltaHistoryBufferData> Super;

  public:
    using Super::Super; /* inherit constructors */

    int update(uint64_t page_number, uint64_t page_offset) {
        Entry *entry = this->find(page_number);
        if (entry) {
            this->set_mru(page_number);
            DeltaHistoryBufferData &data = entry->data;
            int delta = page_offset - data.last_addr;
            if (delta == 0)
                return 0;
            data.deltas.insert(delta);
            data.last_addr = page_offset;
            data.num_times_used += 1;
            return delta;
        } else {
            /* If no matching entry is found (DHB miss), then a not-Most Recently Used (nMRU) DHB entry is evicted and
             * assigned to the new page number (nMRU replacement policy). */
            this->insert(page_number, {page_offset, ShiftRegister(4), 0, 1});
            this->set_mru(page_number);
            return -1;
        }
    }
};

class OffsetPredictionTableData {
  public:
    int pred;
    bool accuracy;
};

class OffsetPredictionTable : public DirectMappedCache<OffsetPredictionTableData> {
    typedef DirectMappedCache<OffsetPredictionTableData> Super;

  public:
    using Super::Super; /* inherit constructors */

    void update(int first_offset, int delta) {
        /* If the OPT prediction matches the observed delta, the accuracy bit is set to 1, or remains 1 if it was
         * already 1. If the OPT prediction does not match the observed delta, the accuracy bit is set to 0. If the
         * accuracy bit was already 0, the old predicted delta is replaced with the new observed delta, and the accuracy
         * bit remains 0. */
        Entry *entry = this->find(first_offset);
        if (entry) {
            OffsetPredictionTableData &data = entry->data;
            if (data.pred == delta) {
                data.accuracy = 1;
            } else {
                if (data.accuracy == 0)
                    data.pred = delta;
                data.accuracy = 0;
            }
        } else {
            this->insert(first_offset, {delta, 0});
        }
    }
};

class DeltaPredictionTableData {
  public:
    int pred;
    SaturatingCounter accuracy;
    /* The delta histories obtained from the DHB are used as the keys, and the delta predictions stored in the DPT are
     * the values. Also, each DPT entry has a 2-bit accuracy counter, and a 1-bit nMRU value. */
    /* Each entry in the table consists of 4 basic elements: a delta history (delta), a delta prediction (pred), a 2-bit
     * accuracy counter, and a single nMRU bit. */
};

class DeltaPredictionTable : public NMRUFullyAssociativeCache<DeltaPredictionTableData> {
    typedef NMRUFullyAssociativeCache<DeltaPredictionTableData> Super;

  public:
    using Super::Super; /* inherit constructors */
};

class DeltaPredictionTables {
  public:
    DeltaPredictionTables(int delta_prediction_table_size)
        /* Our DPT implementation uses a set of 3 DPT tables, allowing for histories up to 3 deltas long. */
        : delta_prediction_table(3, DeltaPredictionTable(delta_prediction_table_size)) {}

    void update(uint64_t page_number, ShiftRegister deltas, int last_predictor) {
        /* last predictor is either 0, 1 or 2 (since there are 3 DPTs) */
        uint64_t key = deltas.get_code(1, last_predictor + 2);
        /* The DHB entry for a page also stores the ID of the DPT table which was most recently used to predict the
         * prefetch candidates for this page. This ID is used to update the accuracy of the DPT */
        DeltaPredictionTable::Entry *entry = delta_prediction_table[last_predictor].find(key);
        if (entry) {
            delta_prediction_table[last_predictor].set_mru(key);
            int last_delta = deltas.get_value(0);
            DeltaPredictionTableData &dpt_data = entry->data;
            /* Based on the accuracy of the previous prediction the accuracy bits of that prediction entry can be
             * updated; incremented in the case of an accurate prediction, decremented otherwise. */
            if (dpt_data.pred == last_delta)
                dpt_data.accuracy.inc();
            else {
                dpt_data.accuracy.dec();
                /* Finally, if the prediction accuracy is sufficiently low, the delta prediction field may be updated to
                 * reflect the new delta. */
                if (dpt_data.accuracy.get_cnt() == 0)
                    dpt_data.pred = last_delta;
                /* Inaccurate predictions in a DPT table T will prompt the promotion of the delta pattern to the next
                 * table T+1. */
                if (last_predictor != 2 && deltas.get_value(last_predictor + 2) != 0) {
                    uint64_t new_key = deltas.get_code(1, last_predictor + 3);
                    /* NOTE: promoted entries are assigned an accuracy of zero */
                    delta_prediction_table[last_predictor + 1].insert(new_key, {deltas.get_value(0)});
                    delta_prediction_table[last_predictor + 1].set_mru(new_key);
                }
            }
        }
        /* If a matching delta pattern is not currently present in any DPT tables, then an entry is created for the
         * latest delta in the shortest-history table. */
        for (int i = 2; i >= 0; i -= 1) {
            DeltaPredictionTable &dpt = delta_prediction_table[i];
            uint64_t key = deltas.get_code(1, i + 2);
            DeltaPredictionTable::Entry *entry = dpt.find(key);
            if (entry)
                return;
        }
        delta_prediction_table[0].insert(deltas.get_code(1, 2), {deltas.get_value(0)});
        delta_prediction_table[0].set_mru(deltas.get_code(1, 2));
    }

    /**
     * @return A pair containing (delta prediction, table index).
     */
    pair<int, int> get_prediction(uint64_t page_number, ShiftRegister deltas) {
        /* When searching for a delta to prefetch, VLDP prefers to use predictions that come from DPT tables that track
         * longer histories. */
        /* DPT lookups may produce matches in more than one of the DPT tables. In these cases, VLDP prioritizes
         * predictions made by the table that uses the longest matching delta history, which maximizes accuracy. */
        for (int i = 2; i >= 0; i -= 1) {
            DeltaPredictionTable &dpt = delta_prediction_table[i];
            uint64_t key = deltas.get_code(0, i + 1);
            DeltaPredictionTable::Entry *entry = dpt.find(key);
            if (entry) {
                dpt.set_mru(key);
                return make_pair(entry->data.pred, i);
            }
        }
        return make_pair(0, 0);
    }

  private:
    vector<DeltaPredictionTable> delta_prediction_table;
};

class VLDP {
  public:
    /* page_size is in blocks */
    VLDP(int page_size, int prefetch_degree, int delta_history_buffer_size, int offset_prediction_table_size,
        int delta_prediction_table_size)
        : page_size(page_size), prefetch_degree(prefetch_degree), delta_history_buffer(delta_history_buffer_size),
          offset_prediction_table(offset_prediction_table_size), delta_prediction_tables(delta_prediction_table_size) {}

    /**
     * @param block_number The block number that caused a PAE (miss or prefetch hit).
     * @return A vector of block numbers that should be prefetched.
     */
    vector<uint64_t> access(uint64_t block_number) {
        uint64_t page_number = block_number / this->page_size;
        int page_offset = block_number % this->page_size;
        int delta = this->delta_history_buffer.update(page_number, page_offset);
        if (delta == 0)
            return vector<uint64_t>();
        DeltaHistoryBufferData &dhb_data = this->delta_history_buffer.find(page_number)->data;
        if (dhb_data.num_times_used == 1) {
            /* On the first access to a page, the OPT is looked up using the page offset, and if the accuracy bit is set
             * for this entry, a prefetch is issued with the predicted delta*/
            OffsetPredictionTable::Entry *entry = this->offset_prediction_table.find(page_offset);
            if (!entry || entry->data.accuracy == 0)
                return vector<uint64_t>();
            else
                return vector<uint64_t>(1, entry->data.pred + page_number * this->page_size);
        } else if (dhb_data.num_times_used == 2) {
            /* On the second access to a page, a delta can be computed and compared with the contents of the OPT. */
            uint64_t first_offset = page_offset - delta;
            this->offset_prediction_table.update(first_offset, delta);
        } else {
            this->delta_prediction_tables.update(page_number, dhb_data.deltas, dhb_data.last_predictor);
        }
        vector<uint64_t> preds;
        ShiftRegister deltas = dhb_data.deltas;
        for (int i = 0; i < this->prefetch_degree; i += 1) {
            pair<int, int> pred = this->delta_prediction_tables.get_prediction(page_number, deltas);
            int delta = pred.first;
            int table_index = pred.second;
            /* Despite the fact that the DPT yields 4 separate predictions in this case, the accuracy is updated only
             * once for the table that issued the original prediction. */
            if (i == 0)
                dhb_data.last_predictor = table_index;
            /* While issuing multi degree prefetches, we only accept predictions of tables 2-3 for degrees greater
             * than 1. */
            if (delta == 0 || (i > 0 && table_index == 0))
                break;
            block_number += delta;
            preds.push_back(block_number);
            deltas.insert(delta);
        }
        return preds;
    }

  private:
    int page_size, prefetch_degree;
    DeltaHistoryBuffer delta_history_buffer;
    OffsetPredictionTable offset_prediction_table;
    DeltaPredictionTables delta_prediction_tables;
};