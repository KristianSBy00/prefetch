#include "opt.h"

unsigned int calculate_opt_adress(unsigned int addr, int delta);

//create global direct mapped OPT cache table
struct opt_table OPT_prefetch[opt_entry_num];

unsigned int calculate_opt_adress(unsigned int addr, int delta)
{
    //printf("\n\nIteration: %d\n", delta);
    //delta unused
    unsigned int calced_addr;

    static unsigned char block_access = 0;

    static unsigned int previous_tag = 0;
    static long previous_addr = 0;
    long current_addr = addr;
    int delta_t; 

    //calculate tag
    unsigned int tmp_Tag = addr >> (int)log2(BLOCK_SIZE);
    tmp_Tag = tmp_Tag >> (int)log2(opt_entry_num);
    //printf("Tag: %x  ", tmp_Tag);

    //first access of the page
    if(tmp_Tag != previous_tag)
    {
        //check validity
        if (OPT_prefetch[tmp_Tag].accuracy == OPT_ENTRY_ACCURATE)
        {
            calced_addr = OPT_prefetch[tmp_Tag].delta;
            //printf("Actual delta: %d\n", calced_addr);
        }
        else
        {
            //Do nothing?
            //Or return delta = 1? (Next Line)
        }
        block_access = 1;
        
    }
    //Second access of the same page
    else if( block_access == 1)
    {
        //calculate delta
        delta_t = current_addr - previous_addr;
        //printf("Actual delta: %d\n", delta_t);

        //matching delta => good prediction
        if (delta_t == OPT_prefetch[tmp_Tag].delta)
        {
            OPT_prefetch[tmp_Tag].accuracy = 1;
            calced_addr = OPT_prefetch[tmp_Tag].delta;
        }
        //the stored mismatches with the current delta
        else
        {
            if (OPT_prefetch[tmp_Tag].accuracy == 0)
                OPT_prefetch[tmp_Tag].delta = delta_t;

            OPT_prefetch[tmp_Tag].accuracy = 0;
        }

        block_access++;
    }
    //3 and 3+ accesses of the same page
    else
    {
        //Do nothing
    }
    
    //Save current datas 
    previous_addr = current_addr;
    previous_tag = tmp_Tag;

    //return prefetchable addr(OFFSET!)
    return calced_addr;
}

void printOPT()
{
    printf("______________________________\n");
    printf("| TAG |   delta   | Accuracy |\n");
    for (int i = 0; i < opt_entry_num; i++)
    {
        printf("| %2x  |     %d     |     %d    |\n",OPT_prefetch[i].Tag,  OPT_prefetch[i].delta, OPT_prefetch[i].accuracy);
    }
    printf("|_____|___________|__________|\n");
}