#include "mem/cache/prefetch/tdt_prefetcher.hh"

#include "mem/cache/prefetch/associative_set_impl.hh"
#include "mem/cache/replacement_policies/base.hh"
#include "params/TDTPrefetcher.hh"


#include "VLDP_prefetcher.c"

#include "vldp.cpp"

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

    //Addr access_addr_p = pfi.getPaddr();

    //Addr pysical = pfi.getPaddr();

    //unsigned long blk_addr = access_addr >> 10;

    //long delta = prefetch_delta(blk_addr) << 10;

    unsigned long block_num = access_addr / blkSize;

    struct stupid prefetch = VLDP_prefetch(block_num);
    
    for(int i = 0; i < 4; i++){
        unsigned long block = prefetch.blocks[i];

        if (block != block_num){
            addresses.push_back(AddrPriority(block * blkSize, 0));

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
        else{
            break;
        }
    }

    /*
    vector<uint64_t> to_prefetch = vldp.access(block_num);

    for(uint64_t i : to_prefetch){
        addresses.push_back(AddrPriority(i * blkSize, 0));

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
    */
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