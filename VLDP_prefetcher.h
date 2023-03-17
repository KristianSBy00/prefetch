#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              134217728u
#define MAIN_MEMORY_SIZE        8589934592u
#define opt_entry_num           MAIN_MEMORY_SIZE/BLOCK_SIZE //=> 64

#define OPT_ENTRY_ACCURATE      1u
#define OPT_ENTRY_INACCURATE    0u

struct DPTEntry {
  int prediction;
  int LRU;
  long delatas[4];
  int page_num;
};


struct entry {
  int valid;
  int i;
  int j;
};

struct prediction {
  int valid;
  long value;
};

struct opt_table
{
    unsigned int Tag;
    int delta;
    unsigned char accuracy;
};

int calculate_opt_adress(unsigned long addr);
void update_LRU(struct entry finds[]);
void nuke_DPTs();
void update_deltas(int page_num, long next_delta, long raw_delta_sequence[]);
struct prediction find_next_delta(int page_num, long delta_seq[]);
long prefetch_delta(unsigned long adress);
unsigned long VLDP_prefetch(unsigned long adress);
unsigned long DPTH_prefetch(unsigned long adress);