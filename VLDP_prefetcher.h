#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              131072u
#define MAIN_MEMORY_SIZE        8388608u
#define opt_entry_num           MAIN_MEMORY_SIZE/BLOCK_SIZE //=> 64

#define OPT_ENTRY_ACCURATE      1u
#define OPT_ENTRY_INACCURATE    0u

#define n 4
#define m 8
#define p 64

struct DPTEntry {
  int prediction;
  int LRU;
  int delatas[n];
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

struct opt_table
{
    unsigned int Tag;
    int delta;
    unsigned char accuracy;
};

int calculate_opt_adress(unsigned int addr);
void update_LRU(struct entry finds[]);
void nuke_DPTs();
void update_deltas(int next_delta, int raw_delta_sequence[]);
struct prediction find_next_delta(int delta_seq[]);
int prefetch_delta(int adress);
unsigned int VLDP_prefetch(int adress);