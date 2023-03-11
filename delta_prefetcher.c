#include <stdio.h>
#include <stdlib.h>

#define n 4
#define m 4

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

int last_adress = -1;
int last_deltas[n];
int new_delta_seq[n];
struct DPTEntry DPTs[n][m];

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


struct prediction prefetch(int adress){
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

int main() {
   printf("Hello!\n");
   struct prediction prefetched;

   prefetched = prefetch(1);
   prefetched = prefetch(2);
   prefetched = prefetch(3);

   prefetched = prefetch(1);
   prefetched = prefetch(2);

   prefetched = prefetch(3);
   prefetched = prefetch(5);
   prefetched = prefetch(7);

   if (prefetched.valid){
      printf("Next: %d\n", prefetched.value);
   }

   return 0;
}