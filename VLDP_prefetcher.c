#include "VLDP_prefetcher.h"

int n = 4;
int m = 1024;

int last_adress_valid = 0;


unsigned long last_adress;
long last_deltas[4];
long new_delta_seq[4];
struct DPTEntry DPTs[4][1024];

struct per_page_DHB_entry per_page_DHB[1024];

unsigned int page_num = 0;
unsigned int prev_page = 0;

//create global direct mapped OPT cache table
struct opt_table OPT_prefetch[opt_entry_num];


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


void update_deltas(int page_num, long next_delta, long raw_delta_sequence[]){
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


struct prediction find_next_delta(int page_num, long delta_seq[]){
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


long prefetch_delta(unsigned long adress){
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

int calculate_opt_adress(unsigned long addr)
{
    //delta unused
    int calced_addr = 0;


    static unsigned char block_access = 0;

    static unsigned int previous_tag = 0;
    static long previous_addr = 0;
    long current_addr = addr;
    int delta_t; 

    //calculate tag
    unsigned int tmp_Tag = addr >> (int)log2(BLOCK_SIZE);
    tmp_Tag = tmp_Tag >> (int)log2(opt_entry_num);

    //first access of the page
    if(tmp_Tag != previous_tag)
    {
        //check validity
        if (OPT_prefetch[tmp_Tag].accuracy == OPT_ENTRY_ACCURATE)
        {
            calced_addr = OPT_prefetch[tmp_Tag].delta;
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

unsigned long DPTH_prefetch(unsigned long adress){
   int delta = prefetch_delta(adress);
   return adress + delta;
}

unsigned long VLDP_prefetch(unsigned long adress){
   int next_delta_DPT;
   int next_delta_OPT;
   unsigned long addr_out;

   addr_out = adress;

   unsigned long reagion = adress >> 12;

   if(reagion != 0){
      next_delta_OPT = calculate_opt_adress(adress); 
   }
   
   //Always returns 0 on first 3 acceses!
   next_delta_DPT = prefetch_delta(adress);
   next_delta_OPT = calculate_opt_adress(adress); 

   if (next_delta_OPT != 0){
      printf("Sending result from opt\n");
      //return adress + next_delta_OPT;
      addr_out = adress + next_delta_OPT;
   }
   if (next_delta_DPT != 0){
      printf("Sending result from delta\n");
      printf("The next from DPTs is: %d\n", next_delta_DPT);
      //return adress + next_delta_DPT;
      addr_out = adress + next_delta_DPT;
   }
   
   return addr_out;

}


/*
int main() {
   unsigned long prefetched;

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);
   prefetched = VLDP_prefetch(4);

   prefetched = VLDP_prefetch(10000000);
   prefetched = VLDP_prefetch(1000000010);
   prefetched = VLDP_prefetch(10000020);

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);

   prefetched = VLDP_prefetch(10000000);
   prefetched = VLDP_prefetch(10000010);
   prefetched = VLDP_prefetch(10000020);

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);

   printf("Switching page!\n");

   //prefetched = VLDP_prefetch(10011111100000);
   printf("Next: %ld\n", prefetched);
   
   prefetched = VLDP_prefetch(10000010);
   prefetched = VLDP_prefetch(858993459-6);

   printf("Next: %ld\n", prefetched);

   return 0;
}
*/


/*
int main() {
   unsigned long prefetched;

   prefetched = DPTH_prefetch(1);
   prefetched = DPTH_prefetch(2);
   prefetched = DPTH_prefetch(4);

   prefetched = DPTH_prefetch(10000000);
   prefetched = DPTH_prefetch(1000000010);
   prefetched = DPTH_prefetch(10000020);

   prefetched = DPTH_prefetch(1);
   prefetched = DPTH_prefetch(2);

   prefetched = DPTH_prefetch(10000000);
   prefetched = DPTH_prefetch(10000010);
   prefetched = DPTH_prefetch(10000020);

   prefetched = DPTH_prefetch(1);
   prefetched = DPTH_prefetch(4);
   prefetched = DPTH_prefetch(7);

   printf("Switching page!\n");

   //prefetched = VLDP_prefetch(10011111100000);
   printf("Next: %ld\n", prefetched);
   
   prefetched = DPTH_prefetch(10000010);
   prefetched = DPTH_prefetch(858993459-6);

   printf("Next: %ld\n", prefetched);

   return 0;
}
*/