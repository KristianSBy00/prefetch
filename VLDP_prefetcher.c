#include "VLDP_prefetcher.h"

int n = HIST_SIZE;
int m = DPT_ENTRYS;
int last_adress_valid = 0;
int page_size = PAGE_SIZE;

struct DPTEntry DPTs[HIST_SIZE][DPT_ENTRYS];
struct DHB_entry DHB[DPT_ENTRYS];

//create global direct mapped OPT cache table
struct opt_table OPT_prefetch[opt_entry_num];

int get_DHB_index(unsigned long page_num){

   int to_remove = 0;

   for (int i = 0; i < page_size; i++){
      if (DHB[i].page_num == page_num){
         return i;
      }

      if (DHB[i].LRU >  DHB[to_remove].LRU){
         to_remove = i;
      }
   }
 
  struct DHB_entry new_entry;
  new_entry.page_num = page_num;
  new_entry.acceced = 0;
  new_entry.LRU = 0;
 
  for(int i = 0; i<n; i++){
   new_entry.deltas[i] = 0;
  }

  for (int i = 0; i < page_size; i++){
   DHB[i].LRU += 1;
  }

  DHB[to_remove] = new_entry;

  return to_remove;

}

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

void print_delta_prediction_table(int i){
   printf("Table %d:\n", i);
   for(int j = 0; j < m; j++){

      printf("Seqence: \n");

      for(int k = 0; k <= i; k++){
         printf("%ld ", DPTs[i][j].delatas[k]);
      }
      printf(", LRU: %d", DPTs[i][j].LRU);
      printf(", prediction: %ld", DPTs[i][j].prediction);
      printf("\n");
   }
}

void print_delta_prediction_tables(){
   for(int i = 0; i < n; i++){
      print_delta_prediction_table(i);
   }
}

void update_delta_prediction_table(int i, int page_num, int next_delta, long raw_deltas[]){
   int listing;
   int victm = 0; 

   for(int j = 0; j < m; j++){
      listing = j; 

      for(int b = 0; b < i+1; b++){
         if (DPTs[i][j].delatas[b] != raw_deltas[b]){
            listing = -1;
         }
      }

      if ( listing != -1 ){
         break;
      }

      if (DPTs[i][j].LRU > DPTs[i][victm].LRU){
         victm = j;
      }
   }

   for(int j = 0; j < m; j++){
      DPTs[i][j].LRU += 1;
   }

   if ( listing != -1 ){
      DPTs[i][listing].LRU = 0;
      return;
   }

   struct DPTEntry new_entry;
   new_entry.LRU = 0;
   new_entry.prediction = next_delta;
   new_entry.page_num = page_num;

   for(int b = 0; b < i+1; b++){
      new_entry.delatas[b + n - 1 - i] = raw_deltas[b + n - 1 - i];
   }

   DPTs[i][victm] = new_entry; 
}

void update_delta_prediction_tables(int page_num, int next_delta, long raw_deltas[]){
   for(int i = 0; i < n; i++){
      update_delta_prediction_table(i, page_num, next_delta, raw_deltas);
   }
}


long find_next_delta(int page_num, long delta_seq[]){
   struct entry finds[n];

   int first_valid = -1;

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
      return 0;
   }

   update_LRU(finds);

   return DPTs[finds[first_valid].i][finds[first_valid].j].prediction;
}

void update_delta_history_buffer(unsigned long index, unsigned long page_offset){
   //Find entry

   long delta = page_offset - DHB[index].last_addr;
   DHB[index].last_addr = page_offset;
   DHB[index].last_predictor = delta;

   if (delta == 0){
      return;
   }

   int new_seq[4];

   for(int i = 0; i < n - 1; i++){
      new_seq[i] = DHB[index].deltas[i+1];
   }

   //DHB[index].last_addr = page_offset;

   new_seq[3] = delta;

   for(int i = 0; i < n; i++){
      DHB[index].deltas[i] = new_seq[i];
   }

   DHB[index].acceced += 1;
   DHB[index].last_predictor = delta;
}

/*
int calculate_opt_adress(unsigned long addr)
{
    //delta unused
    //int calced_addr = 0;

    static unsigned char block_access = 0;
    static unsigned int previous_tag = 0;
    static long previous_addr = 0;
    long current_addr = addr;
    int delta_t; 


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
*/

unsigned long VLDP_prefetch(unsigned long block_number){
   unsigned long page_num = block_number / page_size;
   int page_offset = block_number % page_size;

   int DHB_index = get_DHB_index(page_num);
   update_delta_history_buffer(DHB_index, page_offset);

   if (DHB[DHB_index].last_predictor == 0){
      return block_number;
   }

   if (DHB[DHB_index].acceced == 1){
      return block_number;
   }
   else if (DHB[DHB_index].acceced == 2){
      return block_number;
   }
   else{
      update_delta_prediction_tables(page_num, DHB[DHB_index].last_predictor, DHB[DHB_index].deltas);
      return block_number + find_next_delta(page_num, DHB[DHB_index].deltas);
   }

}


/*
int main() {
   unsigned long prefetched;

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);
   prefetched = VLDP_prefetch(3);
   prefetched = VLDP_prefetch(4);


   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(3); //+2
   prefetched = VLDP_prefetch(5); //+2
   prefetched = VLDP_prefetch(7);//+2

   prefetched = VLDP_prefetch(63);//+2
   prefetched = VLDP_prefetch(64);//+2
   prefetched = VLDP_prefetch(65);//+2
   prefetched = VLDP_prefetch(66);//+2

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