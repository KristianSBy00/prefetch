#include "mem/cache/prefetch/tdt_prefetcher.hh"

#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "params/TDTPrefetcher.hh"

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

struct DPTEntry {
  int next_delta;
  int prev_delatas[];
};

//Global variables


int n = 4; //Number of delta prediction tables
int m = 64; //Number of entries in delta predicton tables
DPTEntry DPTs[n][m][]; //Delta predicton tables
Addr last_adress = void;
int last_deltas[n];

void
TDTPrefetcher::calculatePrefetch(const PrefetchInfo &pfi,
                                 std::vector<AddrPriority> &addresses)
{
    Addr access_addr = pfi.getAddr();
    Addr access_pc = pfi.getPC();
    int context = 0;

    last_deltas[n];
    tmp[n]

    for(int i = 0; i < n-1; i++){
        tmp[i] = last_deltas[i+1];
    }
    int current_delta = access_addr - last_adress;
    tmp[n-1] = last_adress;

    last_deltas = tmp;

    for (int i = 0; i < n; i++ ){
        DPTs[i] = [i+1];
    }

    int next_delta = void;

    for(int i = n; i > 0; i--){
        for(int j = 0; j < m; j++){

            found = true;

            for (int k = 0; k < i; k++){
                if (DPTs[i][j].prev_delatas[k] != last_deltas[k]){
                    found = false;
                }
            }
            if (found && (next_delta == void)){
                next_delta = DPTs[i][j].next_delta;
            }
        }
    }

    int prefetc_addr = access_addr + next_delta;

    // Next line prefetching
    addresses.push_back(AddrPriority(access_addr + blkSize, 0));

    // Get matching storage of entries
    // Context is 0 due to single-threaded application
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
    last_adress = access_addr;
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
