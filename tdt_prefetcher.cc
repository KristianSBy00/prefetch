#include "mem/cache/prefetch/tdt_prefetcher.hh"

#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "params/TDTPrefetcher.hh"


//#include "VLDP_prefetcher.c"

struct DPTEntry {
  int prediction;
  int LRU;
  int delatas[4];
  int page_num;
};


struct entry {
  int valid;
  int i;
  int j;
};

struct prediction {
  int valid;
  int value;
};


int n = 3;
int m = 16;

int last_adress_valid = 0;
unsigned long last_adress;
int last_deltas[3];
int new_delta_seq[3];
struct DPTEntry DPTs[3][16];

unsigned int page_num = 0;
unsigned int prev_page = 0;

//create global direct mapped OPT cache table

void update_LRU(struct entry finds[]){
   for(int i = 0; i < n; i++){
      for(int j = 0; j < m; j++)
         DPTs[i][j].LRU += 1;
   }

   for(int x = 0; x < n; x++){
      if (finds[x].valid == 1){
         DPTs[finds[x].i][finds[x].j].LRU = 0;
      }
   }
}


void update_deltas(int page_num, int next_delta, int raw_delta_sequence[]){
   for(int i = 0; i < n; i++){
      int victm = 0;

      struct DPTEntry new_entry;
      new_entry.LRU = 0;
      new_entry.prediction = next_delta;
      new_entry.page_num = page_num;

      for(int b = 0; b < i+1; b++){
         new_entry.delatas[b + n - 1 - i] = raw_delta_sequence[b + n - 1 - i];
      }

      for(int j = 0; j < m; j++){
         if (DPTs[i][j].LRU > DPTs[i][victm].LRU){
            victm = j;
         }
      }

      DPTs[i][victm] = new_entry;
   }
}


struct prediction find_next_delta(int page_num, int delta_seq[]){
   struct entry finds[n];

   int first_valid = -1;

   struct prediction pred;
   pred.valid = 0;
   pred.value = 0;

   int found_any = 0;

   for (int bi = 0; bi<n; bi++){
      int i = n - bi - 1;

      for(int j = 0; j<m; j++){
         finds[i].valid = 1;

         //Block other pages
         if(DPTs[i][j].page_num != page_num){
            finds[i].valid = 0;
            continue;
         }

         for(int k = 0; k<i+1; k++){
            if(DPTs[i][j].delatas[k+bi] != delta_seq[k+bi]){
               finds[i].valid = 0;
            }
         }

         if (finds[i].valid == 1){
            finds[i].i = i;
            finds[i].j = j;
            found_any = 1;

            if (first_valid == -1){
               first_valid = i;
            }

            break; 
         }
      }
   }

   if (found_any == 0){
      return pred;
   }

   update_LRU(finds);

   pred.valid = 1;
   pred.value = DPTs[finds[first_valid].i][finds[first_valid].j].prediction;

   return pred;
}


int prefetch_delta(unsigned long adress){
   int page_num = 0;

   //first access of the page
   if(page_num != prev_page){
      last_adress_valid = 0;
      prev_page = page_num;
      return 0;
   }

   prev_page = page_num;

   if (last_adress_valid == 0){
      last_adress = adress;
      last_adress_valid = 1;
      return 0;
   }

   int new_delta = adress - last_adress;

   for(int i = 0; i < n-1; i++){
      new_delta_seq[i] = last_deltas[i+1];
   }

   new_delta_seq[n-1] = new_delta;

   update_deltas(page_num, new_delta, last_deltas);

   struct prediction next_delta_prediction = find_next_delta(page_num, new_delta_seq);

   for(int i = 0; i < n; i++){
      last_deltas[i] = new_delta_seq[i];
   }

   last_adress = adress;

   //Might need more gurding
   if ( (next_delta_prediction.valid == 0) ){
      return 0;
   }

   return next_delta_prediction.value;
}






namespace gem5
{

GEM5_DEPRECATED_NAMESPACE(Prefetcher, prefetch);
namespace prefetch
{

TDTPrefetcher::TDTEntry::TDTEntry()
    : TaggedEntry()
{
    invalidate();
}

void
TDTPrefetcher::TDTEntry::invalidate()
{
    TaggedEntry::invalidate();
}

TDTPrefetcher::TDTPrefetcher(const TDTPrefetcherParams &params)
    : Queued(params),
      pcTableInfo(params.table_assoc, params.table_entries,
                  params.table_indexing_policy,
                  params.table_replacement_policy)
    {}

TDTPrefetcher::PCTable*
TDTPrefetcher::findTable(int context)
{
    auto it = pcTables.find(context);
    if (it != pcTables.end())
        return &it->second;

    return allocateNewContext(context);
}

TDTPrefetcher::PCTable*
TDTPrefetcher::allocateNewContext(int context)
{
    auto insertion_result = pcTables.insert(
    std::make_pair(context,
        PCTable(pcTableInfo.assoc, pcTableInfo.numEntries,
        pcTableInfo.indexingPolicy, pcTableInfo.replacementPolicy,
        TDTEntry())));

    return &(insertion_result.first->second);
}

void
TDTPrefetcher::calculatePrefetch(const PrefetchInfo &pfi,
                                 std::vector<AddrPriority> &addresses)
{
    
    Addr access_addr = pfi.getAddr();
    Addr access_pc = pfi.getPC();
    int context = 0;

    Addr access_addr_p = pfi.getPaddr();

    //Addr pysical = pfi.getPaddr();

    int offset = log2(blkSize);

    unsigned long bad_name = access_addr >> offset;

    int delta = prefetch_delta(bad_name) << offset;

    int offset_delta = delta;

    unsigned long new_guy = access_addr + offset_delta;


    if (delta == 0){
        addresses.push_back(AddrPriority(access_addr + blkSize, 0));

        PCTable* pcTable = findTable(context);

        // Get matching entry from PC
        TDTEntry *entry = pcTable->findEntry(access_pc, false);

        // Check if you have entry
        if (entry != nullptr) {
            // There is an entry
        } else {
            // No entry
        }

        // *Add* new entry
        // All entries exist, you must replace previous with new data
        // Find replacement victim, update info
        TDTEntry* victim = pcTable->findVictim(access_pc);
        victim->lastAddr = access_addr;
        pcTable->insertEntry(access_pc, false, victim);
    }
}

uint32_t
TDTPrefetcherHashedSetAssociative::extractSet(const Addr pc) const
{
    const Addr hash1 = pc >> 1;
    const Addr hash2 = hash1 >> tagShift;
    return (hash1 ^ hash2) & setMask;
}

Addr
TDTPrefetcherHashedSetAssociative::extractTag(const Addr addr) const
{
    return addr;
}

}
}