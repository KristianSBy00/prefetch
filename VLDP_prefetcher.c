#include "VLDP_prefetcher.h"

int last_adress = -1;
int last_deltas[n];
int new_delta_seq[n];
struct DPTEntry DPTs[n][m];

int page_counter = 0;

int my_previous_tag = -1;

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


void nuke_DPTs(){
   printf("Nuke go BOOOM!!\n");

   for(int i = 0; i < n; i++){
      last_deltas[i] = 0;
      new_delta_seq[i] = 0;
      struct DPTEntry fresh_new;

      for(int j = 0; j < m; j++){
         DPTs[i][j] = fresh_new;
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


int prefetch_delta(int adress){

   int tmp_Tag = adress >> (int)log2(BLOCK_SIZE);
   tmp_Tag = tmp_Tag >> (int)log2(opt_entry_num);

   //first access of the page
   if(tmp_Tag != my_previous_tag){
      nuke_DPTs();
   }

   my_previous_tag = tmp_Tag;

   if (last_adress == -1){
      printf("Initializing prefetcher!\n");
      last_adress = adress;
      return 0;
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
      //printf("Nothing to prefetch, do dsiplacment insted!\n");
      //return pred;
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

unsigned long VLDP_prefetch(unsigned long adress){


   int next_delta_DPT;
   int next_delta_OPT;
   unsigned long addr_out;
   
   //Always returns 0 on first 3 acceses!
   next_delta_DPT = prefetch_delta(adress);
   next_delta_OPT = calculate_opt_adress(adress); 

   addr_out = adress + 1;

   if (next_delta_OPT != 0){
      printf("Sending result from opt\n");
      //return adress + next_delta_OPT;
      addr_out =adress + next_delta_OPT;
   }
   if (next_delta_DPT != 0){
      printf("Sending result from delta\n");
      printf("The next from DPTs is: %d\n", next_delta_DPT);
      //return adress + next_delta_DPT;
      addr_out = adress + next_delta_DPT;
   }
   

   //Remember to mke sure we dont acces adress that is to larege!!!!

   return addr_out;

}


/*int main() {
   int prefetched;

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);
   prefetched = VLDP_prefetch(4);

   prefetched = VLDP_prefetch(10000000);
   prefetched = VLDP_prefetch(10000010);
   prefetched = VLDP_prefetch(10000020);

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);

   prefetched = VLDP_prefetch(10000000);
   prefetched = VLDP_prefetch(10000010);
   prefetched = VLDP_prefetch(10000020);

   prefetched = VLDP_prefetch(1);
   prefetched = VLDP_prefetch(2);

   printf("Switching page!\n");

   prefetched = VLDP_prefetch(10000000);
   printf("Next: %d\n", prefetched);
   prefetched = VLDP_prefetch(10000010);
   prefetched = VLDP_prefetch(10000020);

   printf("Next: %d\n", prefetched);

   return 0;
}
*/