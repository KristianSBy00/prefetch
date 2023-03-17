#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              4096u
#define MAIN_MEMORY_SIZE        8589934592u
#define opt_entry_num           2048

#define OPT_ENTRY_ACCURATE      1u
#define OPT_ENTRY_INACCURATE    0u

#define n 4
#define m 2048
#define p 64

struct DPTEntry {
  long prediction;
  long LRU;
  long delatas[n];
};


struct entry {
  long valid;
  long i;
  long j;
};

struct prediction {
  long valid;
  long value;
};

struct opt_table
{
    unsigned long Tag;
    long delta;
    unsigned char accuracy;
};

long calculate_opt_delta(unsigned long addr);
void update_LRU(struct entry finds[]);
void nuke_DPTs();
void update_deltas(long next_delta, long raw_delta_sequence[]);
struct prediction find_next_delta(long delta_seq[]);
long prefetch_delta(long adress);
unsigned long VLDP_prefetch(unsigned long adress);
unsigned long mylog2 (unsigned long val);