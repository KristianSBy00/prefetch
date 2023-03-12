#include "VLDP_prefetcher.h"

int last_adress = -1;
int last_deltas[n];
int new_delta_seq[n];
struct DPTEntry DPTs[n][m];

unsigned int calculate_opt_adress(unsigned int addr, int delta);

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


void update_deltas(int next_delta, int raw_delta_sequence[]){
   for(int i = 0; i < n; i++){
      int victm = 0;
      int delta_seqence[4];

      struct DPTEntry new_entry;
      new_entry.LRU = 0;
      new_entry.prediction = next_delta;

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


struct prediction find_next_delta(int delta_seq[]){
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
         for(int k = 0; k<i+1; k++){
            if(DPTs[i][j].delatas[k+bi] != delta_seq[k+bi]){
               finds[i].valid = 0;
            }
         }

         if (finds[i].valid == 1){
            printf("Found a match!\n");
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


struct prediction prefetch_delta(int adress){
   struct prediction pred;
   pred.valid = 0;
   pred.value = 0;

   if (last_adress == -1){
      printf("Initializing prefetcher!\n");
      last_adress = adress;
      return pred;
   }

   int new_delta = adress - last_adress;

   for(int i = 0; i < n-1; i++){
      new_delta_seq[i] = last_deltas[i+1];
   }

   new_delta_seq[n-1] = new_delta;

   update_deltas(new_delta, last_deltas);

   struct prediction next_delta_prediction = find_next_delta(new_delta_seq);

   for(int i = 0; i < n; i++){
      last_deltas[i] = new_delta_seq[i];
   }

   last_adress = adress;

   //Might need more gurding
   if ( (next_delta_prediction.valid == 0) ){
      printf("Nothing to prefetch, do dsiplacment insted!\n");
      return pred;
   }

   pred.valid = 1;
   pred.value = next_delta_prediction.value + adress;

   return pred;
}

int calculate_opt_adress(unsigned int addr)
{
    //delta unused
    int calced_addr;

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

int VLDP_prefetch(int adress){

   int next_delta;
   
   next_delta = prefetch_delta(adress);

   if (next_delta != 0){
      return next_delta;
   }

   next_delta = calculate_opt_adress(adress);

   return next_delta;
   

}

int main() {
   struct prediction prefetched;

   prefetched = prefetch(1);
   prefetched = prefetch(2);
   prefetched = prefetch(3);

   prefetched = prefetch(100);
   prefetched = prefetch(110);
   prefetched = prefetch(120);

   prefetched = prefetch(1);
   prefetched = prefetch(2);

   if (prefetched.valid){
      printf("Next: %d\n", prefetched.value);
   }

   return 0;
}