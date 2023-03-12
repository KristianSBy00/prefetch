#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              131 072u
#define MAIN_MEMORY_SIZE        8 388 608u
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
