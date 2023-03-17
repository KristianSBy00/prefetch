#include "VLDP_prefetcher.h"

long last_adress = -1;
long last_deltas[n];
long new_delta_seq[n];
struct DPTEntry DPTs[n][m];

long page_counter = 0;

long my_previous_tag = -1;

//create global direct mapped OPT cache table
struct opt_table OPT_prefetch[opt_entry_num];

void update_LRU(struct entry finds[]){
   for(long i = 0; i < n; i++){
      for(long j = 0; j < m; j++)
         DPTs[i][j].LRU += 1;
   }

   for(long x = 0; x < n; x++){
      if (finds[x].valid == 1){
         DPTs[finds[x].i][finds[x].j].LRU = 0;
      }
   }
}


void nuke_DPTs(){
   //printf("Nuke go BOOOM!!\n");

   for(long i = 0; i < n; i++){
      last_deltas[i] = 0;
      new_delta_seq[i] = 0;
      struct DPTEntry fresh_new;

      for(long j = 0; j < m; j++){
         DPTs[i][j] = fresh_new;
      } 
   }
}

void update_deltas(long next_delta, long raw_delta_sequence[]){
   for(long i = 0; i < n; i++){
      long victm = 0;

      struct DPTEntry new_entry;
      new_entry.LRU = 0;
      new_entry.prediction = next_delta;

      for(long b = 0; b < i+1; b++){
         new_entry.delatas[b + n - 1 - i] = raw_delta_sequence[b + n - 1 - i];
      }

      for(long j = 0; j < m; j++){
         if (DPTs[i][j].LRU > DPTs[i][victm].LRU){
            victm = j;
         }
      }

      DPTs[i][victm] = new_entry;
   }
}


struct prediction find_next_delta(long delta_seq[]){
   struct entry finds[n];

   long first_valid = -1;

   struct prediction pred;
   pred.valid = 0;
   pred.value = 0;

   long found_any = 0;

   for (long bi = 0; bi<n; bi++){
      long i = n - bi - 1;

      for(long j = 0; j<m; j++){
         finds[i].valid = 1;
         for(long k = 0; k<i+1; k++){
            if(DPTs[i][j].delatas[k+bi] != delta_seq[k+bi]){
               finds[i].valid = 0;
            }
         }

         if (finds[i].valid == 1){
            //printf("Found a match!\n");
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


long prefetch_delta(long adress){

   adress = adress >> 6;
   //calculate region
    //shift offset
    //unsigned long tmp_Tag = addr >> 6;
    //shift index
   unsigned long tmp_Tag = adress >> mylog2(BLOCK_SIZE);
    //shift region
   tmp_Tag = tmp_Tag >> mylog2(opt_entry_num);

   //first access of the region
   if(tmp_Tag != my_previous_tag){
      nuke_DPTs();
   }

   my_previous_tag = tmp_Tag;

   if (last_adress == -1){
      //printf("Initializing prefetcher!\n");
      last_adress = adress;
      return 0;
   }

   long new_delta = adress - last_adress;

   for(long i = 0; i < n-1; i++){
      new_delta_seq[i] = last_deltas[i+1];
   }

   new_delta_seq[n-1] = new_delta;

   update_deltas(new_delta, last_deltas);

   struct prediction next_delta_prediction = find_next_delta(new_delta_seq);

   for(long i = 0; i < n; i++){
      last_deltas[i] = new_delta_seq[i];
   }

   last_adress = adress;

   //Might need more gurding
   if ( (next_delta_prediction.valid == 0) ){
      ////printf("Nothing to prefetch, do dsiplacment insted!\n");
      //return pred;
      return 0;
   }

   return next_delta_prediction.value;
}

long calculate_opt_delta(unsigned long addr)
{
    //delta unused
    long calced_delta = 0;

    static unsigned char region_access = 0;

    static unsigned long previous_tag = 0;
    static long previous_addr = 0;
    long current_addr = addr >> 6;
    long delta_t; 

    //calculate region
    //shift offset
    //unsigned long tmp_Tag = addr >> 6;
    //shift index
    unsigned long tmp_Tag = addr >> mylog2(BLOCK_SIZE);
    //shift region
    tmp_Tag = tmp_Tag >> mylog2(opt_entry_num);

    //first access of the page
    if(tmp_Tag != previous_tag)
    {
        //check validity
        if (OPT_prefetch[tmp_Tag].accuracy == OPT_ENTRY_ACCURATE)
        {
            calced_delta = OPT_prefetch[tmp_Tag].delta;
        }
        else
        {
            //Do nothing
        }
        region_access = 1;
        
    }
    //Second access of the same page
    else if( region_access == 1)
    {
        //calculate delta
        delta_t = current_addr - previous_addr;
        ////printf("Actual delta: %d\n", delta_t);

        //matching delta => good prediction
        if (delta_t == OPT_prefetch[tmp_Tag].delta)
        {
            OPT_prefetch[tmp_Tag].accuracy = 1;
            calced_delta = OPT_prefetch[tmp_Tag].delta;
        }
        //the stored mismatches with the current delta
        else
        {
            if (OPT_prefetch[tmp_Tag].accuracy == 0)
                OPT_prefetch[tmp_Tag].delta = delta_t;

            OPT_prefetch[tmp_Tag].accuracy = 0;
        }

        region_access++;
    }
    //3 and 3+ accesses of the same page
    else
    {
        //Do nothing
    }
    
    //Save current datas 
    previous_addr = current_addr;
    previous_tag = tmp_Tag;

    //return prefetchable delta(LINE OFFSET!)
    return calced_delta;
}

unsigned long VLDP_prefetch(unsigned long adress){


   long next_delta_DPT;
   long next_delta_OPT;
   unsigned long addr_out;
   
   //Always returns 0 on first 3 acceses!
   next_delta_DPT = prefetch_delta(adress) << 6;
   next_delta_OPT = calculate_opt_delta(adress); 

   addr_out = adress + 1;

   if (next_delta_OPT != 0){
      //printf("Sending result from opt\n");
      //return adress + next_delta_OPT;
      addr_out = ((adress >> 6) + next_delta_OPT) << 6;
   }
   if (next_delta_DPT != 0){
      //printf("Sending result from delta\n");
      //printf("The next from DPTs is: %d\n", next_delta_DPT);
      //return adress + next_delta_DPT;
      addr_out = ((adress >> 6) + next_delta_DPT) << 6;
   }
   
   //E


   //Remember to mke sure we dont acces adress that is to larege!!!!

   return addr_out;

}


unsigned long mylog2 (unsigned long val) {
    if (val == 0) return UINT_MAX;
    if (val == 1) return 0;
    unsigned long ret = 0;
    while (val > 1) {
        val >>= 1;
        ret++;
    }
    return ret;
}

/*long main() {
   long prefetched;

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