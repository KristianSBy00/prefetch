#include "VLDP_prefetcher.h"

int n = HIST_SIZE;
int m = DPT_ENTRYS;
int page_size = PAGE_SIZE;

struct DPTEntry DPTs[HIST_SIZE][DPT_ENTRYS];
struct DHB_entry DHB[DPT_ENTRYS];
//create global direct mapped OPT cache table
struct opt_table OPT_prefetch[opt_entry_num];

int get_DHB_index(unsigned long page_num){

   int to_remove = 0;

   for (int i = 0; i < DPT_ENTRYS; i++){
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

  for (int i = 0; i < DPT_ENTRYS; i++){
      DHB[i].LRU += 1;
  }

  DHB[to_remove] = new_entry;

  return to_remove;

}

void update_LRU(struct entry finds[]){
   for(int i = 0; i < n; i++){
      for(int j = 0; j < m; j++)
         DPTs[i][j].LRU++;
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
      printf(", page_num: %d", DPTs[i][j].page_num);
      printf("\n");
   }
}

void print_delta_prediction_tables(){
   for(int i = 0; i < n; i++){
      print_delta_prediction_table(i);
   }
}

void update_delta_prediction_table(int i, int page_num, long raw_deltas[]){
   int listing;
   int victm = 0; 

   for(int j = 0; j < m; j++){
      listing = j; 

      for(int b = 0; b < i+1; b++){
         if (DPTs[i][j].delatas[b] != raw_deltas[3 - i + b]){
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
      DPTs[i][j].LRU++;
   }

   if ( listing != -1 ){
      DPTs[i][listing].LRU = 0;
      return;
   }

   struct DPTEntry new_entry;
   new_entry.LRU = 0;
   new_entry.prediction = raw_deltas[4];
   new_entry.page_num = page_num;

   for(int b = 0; b < i+1; b++){
      new_entry.delatas[b] = raw_deltas[3 - i + b];
   }

   DPTs[i][victm] = new_entry; 
}

void update_delta_prediction_tables(int page_num, long raw_deltas[]){
   for(int i = 0; i < n; i++){
      update_delta_prediction_table(i, page_num, raw_deltas);
   }
}


long find_next_delta(int page_num, int speculative, long delta_seq[]){
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

         for(int k = 0; k<=i; k++){
            if(DPTs[i][j].delatas[k] != delta_seq[k+bi]){
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

   if(speculative == 0){
      update_LRU(finds);
   }
   return DPTs[finds[first_valid].i][finds[first_valid].j].prediction;
}

long update_delta_history_buffer(unsigned long index, unsigned long page_offset){
   //Find entry

   long delta = page_offset - DHB[index].last_referenced_block;
   DHB[index].last_referenced_block = page_offset;

   if (delta == 0){
      return 0;
   }

   int new_seq[5];

   for(int i = 0; i < n; i++){
      new_seq[i] = DHB[index].deltas[i+1];
   }

   //DHB[index].last_addr = page_offset;

   new_seq[4] = delta;

   for(int i = 0; i <= n; i++){
      DHB[index].deltas[i] = new_seq[i];
   }

   DHB[index].acceced += 1;
   DHB[index].last_predictor = delta;

   return delta;
}


void a_update_opt(int offset, long delt){
   if ( OPT_prefetch[offset].delta == delt ){
      OPT_prefetch[offset].accuracy = 1;
   }
   else{
      if (OPT_prefetch[offset].accuracy == 0){
         OPT_prefetch[offset].delta = delt;
         OPT_prefetch[offset].accuracy = 0;
      }
   }
}

struct stupid VLDP_prefetch(unsigned long block_number){
   unsigned long page_num = block_number / page_size;
   int page_offset = block_number % page_size;

   int DHB_index = get_DHB_index(page_num);
   long delta = update_delta_history_buffer(DHB_index, page_offset);
   
   struct stupid out;

   for(int i = 0; i < DEGREE; i++){
      out.blocks[i] = block_number;
   }

   //printf("block_number: %ld, delta: %ld\n", block_number, delta);

   if (delta == 0){
      return out;
   }

   if (DHB[DHB_index].acceced == 1){
      if (OPT_prefetch[page_offset].accuracy == 1){
         out.blocks[0] = OPT_prefetch[page_offset].delta + block_number;
         return out;
      }
      return out;
   }
   else if (DHB[DHB_index].acceced == 2){
      int first_offset = (page_offset - delta) % opt_entry_num;
      a_update_opt(first_offset, delta);
   }
   else{
      update_delta_prediction_tables(page_num, DHB[DHB_index].deltas);
      //print_delta_prediction_tables();
      //print_delta_prediction_tables();
      
      /*
      printf("DHB: {");
      for (int i = 0; i < 4; i++){
         printf("%ld, ", DHB[DHB_index].deltas[i]);
      }
      */
      //printf("}, prediction: %ld\n", delta);
      /*
      update_delta_prediction_tables(page_num, DHB[DHB_index].last_predictor, DHB[DHB_index].deltas);
      print_delta_prediction_tables();

      update_delta_prediction_tables(page_num, delta, DHB[DHB_index].deltas);
      print_delta_prediction_tables();
      printf("page num: %ld, {", page_num);
      for (int i = 0; i < 4; i++){
         printf("%ld, ", DHB[DHB_index].deltas[i]);
      }
      printf("}\n");
      */

      int counter = 0;

      long new_seq[4]; 
      long tmp[4]; 

      for(int i = 0; i < 4; i++){
         new_seq[i] = DHB[DHB_index].deltas[i+1];
      }

      delta = find_next_delta(page_num, 0, new_seq);

      unsigned long current_block = block_number;

      while ( (delta != 0) & ( counter < DEGREE)  ){
         current_block += delta;
         out.blocks[counter] = current_block;
         delta = find_next_delta(page_num, 1, new_seq);

         for(int i = 0; i < 3; i++){
            tmp[i] = new_seq[i+1];
         }

         tmp[3] = delta;

         for(int i = 0; i < 4; i++){
            new_seq[i] = tmp[i];
         }
         counter++;
      }

      return out;
   }

}

/*
int main() {
   unsigned int *prefetched;

   struct stupid prefetch;

   prefetch = VLDP_prefetch(0);//
   prefetch = VLDP_prefetch(0);//+1
   prefetch = VLDP_prefetch(0);//+1
   prefetch = VLDP_prefetch(1);//+1
   prefetch = VLDP_prefetch(2);//+1
   prefetch = VLDP_prefetch(3);//+1
   prefetch = VLDP_prefetch(4);//+1
   prefetch = VLDP_prefetch(5);//+1

   printf("Next: {");

   for(int i = 0; i < DEGREE; i++){
      printf("%ld, ", prefetch.blocks[i]);
   }
   printf("}\n");

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