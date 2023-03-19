#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              134217728
#define page_bytes              4096
#define MAIN_MEMORY_SIZE        8589934592
#define opt_entry_num           64 //=> 64

#define OPT_ENTRY_ACCURATE      1u
#define OPT_ENTRY_INACCURATE    0u

#define PAGE_SIZE               64
#define HIST_SIZE               4
#define DPT_ENTRYS              8
#define DEGREE                  8


struct DHB_entry{
  unsigned long last_addr;
  unsigned long page_num;
  int last_referenced_block;
  //int last_predictor;
  int num_times_used;
  int LRU;
  int acceced;
  long deltas[5];
};



struct DPTEntry {
  long prediction;
  int LRU;
  long delatas[4];
  int page_num;
};

struct stupid {
  unsigned long blocks[DEGREE];
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

int get_DHB_index(unsigned long page_num);
void update_opt(long offset, long delt);
void update_LRU(struct entry finds[]);
void nuke_DPTs();
void update_delta_prediction_tables(int page_num, long raw_deltas[]);
long find_next_delta(int page_num, int speculative, long delta_seq[]);
struct stupid VLDP_prefetch(unsigned long adress);
unsigned long DPTH_prefetch(unsigned long adress);