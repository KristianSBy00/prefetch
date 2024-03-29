/* VLDP [https://www.cs.utah.edu/~rajeev/pubs/micro15m.pdf] */

#include <bits/stdc++.h>

using namespace std;

class Table {
  public:
    Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

    void set_row(int row, const vector<string> &data) {
        assert(data.size() == this->width);
        for (unsigned col = 0; col < this->width; col += 1)
            this->set_cell(row, col, data[col]);
    }

    void set_col(int col, const vector<string> &data) {
        assert(data.size() == this->height);
        for (unsigned row = 0; row < this->height; row += 1)
            this->set_cell(row, col, data[row]);
    }

    void set_cell(int row, int col, string data) {
        assert(0 <= row && row < (int)this->height);
        assert(0 <= col && col < (int)this->width);
        this->cells[row][col] = data;
    }

    void set_cell(int row, int col, double data) {
        this->oss.str("");
        this->oss << setw(11) << fixed << setprecision(8) << data;
        this->set_cell(row, col, this->oss.str());
    }

    void set_cell(int row, int col, uint64_t data) {
        this->oss.str("");
        this->oss << setw(11) << std::left << data;
        this->set_cell(row, col, this->oss.str());
    }

    string to_string() {
        vector<int> widths;
        for (unsigned i = 0; i < this->width; i += 1) {
            int max_width = 0;
            for (unsigned j = 0; j < this->height; j += 1)
                max_width = max(max_width, (int)this->cells[j][i].size());
            widths.push_back(max_width + 2);
        }
        string out;
        out += Table::top_line(widths);
        out += this->data_row(0, widths);
        for (unsigned i = 1; i < this->height; i += 1) {
            out += Table::mid_line(widths);
            out += this->data_row(i, widths);
        }
        out += Table::bot_line(widths);
        return out;
    }

    string data_row(int row, const vector<int> &widths) {
        string out;
        for (unsigned i = 0; i < this->width; i += 1) {
            string data = this->cells[row][i];
            data.resize(widths[i] - 2, ' ');
            out += " | " + data;
        }
        out += " |\n";
        return out;
    }

    static string top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

    static string mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

    static string bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

    static string line(const vector<int> &widths, string left, string mid, string right) {
        string out = " " + left;
        for (unsigned i = 0; i < widths.size(); i += 1) {
            int w = widths[i];
            for (int j = 0; j < w; j += 1)
                out += "─";
            if (i != widths.size() - 1)
                out += mid;
            else
                out += right;
        }
        return out + "\n";
    }

  private:
    unsigned width;
    unsigned height;
    vector<vector<string>> cells;
    ostringstream oss;
};

template <class T> class InfiniteCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        if (entry)
            entry->valid = false;
        this->last_erased_entry = *entry;
        int num_erased = this->entries.erase(key);
        assert(entry ? num_erased == 1 : num_erased == 0);
        return &this->last_erased_entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        entries[key] = {key, 0, key, true, data};
        return {};
    }

    Entry *find(uint64_t key) {
        auto it = this->entries.find(key);
        if (it == this->entries.end())
            return nullptr;
        Entry &entry = (*it).second;
        assert(entry.tag == key && entry.valid);
        return &entry;
    }

  protected:
    Entry last_erased_entry;
    unordered_map<uint64_t, Entry> entries;
};

template <class T> class SetAssociativeCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    SetAssociativeCache(int size, int num_ways)
        : size(size), num_ways(num_ways), num_sets(size / num_ways), entries(num_sets, vector<Entry>(num_ways)),
          cams(num_sets) {
        assert(size % num_ways == 0);
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                entries[i][j].valid = false;
    }

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        int num_erased = cam.erase(tag);
        if (entry)
            entry->valid = false;
        assert(entry ? num_erased == 1 : num_erased == 0);
        return entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        vector<Entry> &set = this->entries[index];
        int victim_way = -1;
        for (int i = 0; i < this->num_ways; i += 1)
            if (!set[i].valid) {
                victim_way = i;
                break;
            }
        if (victim_way == -1) {
            victim_way = this->select_victim(index);
        }
        Entry &victim = set[victim_way];
        Entry old_entry = victim;
        victim = {key, index, tag, true, data};
        auto &cam = cams[index];
        if (old_entry.valid) {
            int num_erased = cam.erase(old_entry.tag);
            assert(num_erased == 1);
        }
        cam[tag] = victim_way;
        return old_entry;
    }

    Entry *find(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        if (cam.find(tag) == cam.end())
            return nullptr;
        int way = cam[tag];
        Entry &entry = this->entries[index][way];
        assert(entry.tag == tag && entry.valid);
        return &entry;
    }

  protected:
    /**
     * @return The way of the selected victim.
     */
    virtual int select_victim(uint64_t index) {
        /* random eviction policy if not overriden */
        return rand() % this->num_ways;
    }

    int size;
    int num_ways;
    int num_sets;
    vector<vector<Entry>> entries;
    vector<unordered_map<uint64_t, int>> cams;
};

template <class T> class LRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    LRUSetAssociativeCache(int size, int num_ways)
        : Super(size, num_ways), lru(this->num_sets, vector<uint64_t>(num_ways)) {}

    void set_mru(uint64_t key) { *this->get_lru(key) = this->t++; }

    void set_lru(uint64_t key) { *this->get_lru(key) = 0; }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        vector<uint64_t> &lru_set = this->lru[index];
        return min_element(lru_set.begin(), lru_set.end()) - lru_set.begin();
    }

    uint64_t *get_lru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        return &this->lru[index][way];
    }

    vector<vector<uint64_t>> lru;
    uint64_t t = 0;
};

template <class T> class NMRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    NMRUSetAssociativeCache(int size, int num_ways) : Super(size, num_ways), mru(this->num_sets) {}

    void set_mru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        this->mru[index] = way;
    }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        int way = rand() % (this->num_ways - 1);
        if (way >= mru[index])
            way += 1;
        return way;
    }

    vector<int> mru;
};

template <class T> class LRUFullyAssociativeCache : public LRUSetAssociativeCache<T> {
    typedef LRUSetAssociativeCache<T> Super;

  public:
    LRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class NMRUFullyAssociativeCache : public NMRUSetAssociativeCache<T> {
    typedef NMRUSetAssociativeCache<T> Super;

  public:
    NMRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class DirectMappedCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    DirectMappedCache(int size) : Super(size, 1) {}
};

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

class CacheData {
  public:
    bool prefetch;
};

class Cache : public LRUSetAssociativeCache<CacheData> {
    typedef LRUSetAssociativeCache<CacheData> Super;

  public:
    /* We simulate VLDP with 1 offset prediction table and 3 delta prediction tables. Each DPT has 64 entries, and the
     * OPT also has 64 entries. The DHB keeps track of the last 16 pages that were accessed by the application. */
    Cache(int size, int block_size, int num_ways, int cores, int page_size, int prefetch_degree, string name,
        int delta_history_buffer_size = 16, int offset_prediction_table_size = 64, int delta_prediction_table_size = 64)
        : Super(size / block_size, num_ways), block_size(block_size),
          /* We assume that each core has its own, separate VLDP. */
          vldp_modules(cores, VLDP(page_size / block_size, prefetch_degree, delta_history_buffer_size,
                                  offset_prediction_table_size, delta_prediction_table_size)),
          name(name) {
        assert(size % block_size == 0);
        assert(page_size % block_size == 0);
        assert(__builtin_popcount(this->num_sets) == 1);
        assert(__builtin_popcount(this->num_ways) == 1);
    }

    void access(uint64_t address, string type, int core) {
        this->stats["Accesses"] += 1;
        uint64_t block_number = address / block_size;
        Entry *entry = this->find(block_number);
        /* VLDP takes action only when there are accesses to the cache that either result in a miss, or when a cache
         * line is accessed that was previously prefetched into the cache. We refer to this subset of requests as
         * Prefetch Activation Events (PAE). */
        bool pae = false;
        if (entry) {
            this->set_mru(block_number);
            if (entry->data.prefetch) {
                this->stats["Prefetch Hits"] += 1;
                entry->data.prefetch = false;
                pae = true; /* prefetch hit */
            }
        } else {
            this->stats["Misses"] += 1;
            this->fetch(block_number, false);
            pae = true; /* miss */
        }
        if (type != "Read Request" || !pae)
            return;
        vector<uint64_t> to_prefetch = vldp_modules[core].access(block_number);
        for (auto &block_number : to_prefetch)
            this->fetch(block_number, true);
    }

    vector<string> get_log_headers() {
        vector<string> log_headers = {"Name"};
        for (auto &x : this->stats)
            log_headers.push_back(x.first);
        return log_headers;
    }

    /*
     *  write log info in specified row of given table.
     */
    void log(Table &table, int row) {
        int col = 0;
        table.set_cell(row, col++, this->name);
        for (auto &x : this->stats)
            table.set_cell(row, col++, x.second);
    }

    uint64_t get_accesses() { return this->stats["Accesses"]; }

  private:
    void fetch(uint64_t block_number, bool prefetch) {
        if (this->find(block_number))
            return;
        if (prefetch)
            this->stats["Prefetches"] += 1;
        Entry old_entry = this->insert(block_number, {prefetch});
        this->set_mru(block_number);
        if (old_entry.valid && old_entry.data.prefetch)
            this->stats["Non-useful Prefetches"] += 1;
    }

    int block_size;
    vector<VLDP> vldp_modules;
    string name;
    unordered_map<string, uint64_t> stats = {
        {"Accesses", 0}, {"Misses", 0}, {"Prefetches", 0}, {"Prefetch Hits", 0}, {"Non-useful Prefetches", 0}};
};