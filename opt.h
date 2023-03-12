#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>    

//in bytes
#define BLOCK_SIZE              32u
#define MAIN_MEMORY_SIZE        512u
#define opt_entry_num           MAIN_MEMORY_SIZE/BLOCK_SIZE

#define OPT_ENTRY_ACCURATE      1u
#define OPT_ENTRY_INACCURATE    0u


struct opt_table
{
    unsigned int Tag;
    short delta;
    unsigned char accuracy;
};

#define IM_HERE     printf("I'M HERE!")