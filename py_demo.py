from dataclasses import dataclass

@dataclass
class DPTEntry:
   prediction: int
   delatas: list
   LRU = 0

class Prefetcher: 
   
   def __init__(self, n, m):
      self.last_adress = None
      self.last_deltas = [None, None, None, None]
      self.DPTs = []
      self.n = n #Number of delta prediction tables
      self.m = m #Number of entries in delta predicton tables
      
      for i in range (0,n):
         self.DPTs.append([])
         for j in range (0,m):
            
            deltas = [None] * self.m
            
            self.DPTs[i].append(DPTEntry(None, deltas))
            
            
   def __uppdate_LRU(self, finds):
      for i in range(0, self.n):
         for j in range(0, self.m):
            self.DPTs[i][j].LRU += 1
            
      for i, j in finds: 
         self.DPTs[i][j].LRU = 0
      
            
   #This function updates the delata 
   def __update_deltas(self, raw_delta_sequence, next_delta):
      print(f"raw_delta_sequence: {raw_delta_sequence}, next_delta: {next_delta}")
      #Exmple:
      
      #raw_delata_seqence = 1,2,4,8
      #n = 4
      
      #i=0: delta seqence = 8
      #i=1: delta seqence = 4,8
      #i=2: delta seqence = 2,4,8
      #i=3: delta seqence = 1,2,4,8
      
      for i in range(0, self.n):
         victem = 0
         
         delta_seqence = [None] * self.n
         
         #n = 4
         # i = 0: b = [0],          append = 3,          diff = 3  n - 1- i
         # i = 1: b = [0, 1],       append = 2, 3,       diff = 2 
         # i = 2: b = [0, 1, 2],    append = 1, 2, 3     diff = 1 
         # i = 3: b = [0, 1, 2, 3], append = 0, 1, 2, 3  diff = 0
         
         
         for b in range (0, i+1):
            delta_seqence[b + self.n - 1 - i] = raw_delta_sequence[b + self.n - 1 - i]
         
         for j in range (1, self.m):
            if (self.DPTs[i][j].LRU > self.DPTs[i][victem].LRU):
               victem = j
               
         self.DPTs[i][victem] = DPTEntry(next_delta, delta_seqence)
                  
   #This function finds the next delta
   def __find_next_delta(self, delta_seq): 
      
      finds = []
      
      print(f"last delats: {self.last_deltas}")
      
      for ib in range (0, self.n): #Itterate over DPTs
         i = self.n - ib - 1 #i -> n-1, n-2, n-3 ... 0,    3, 2, 1, 0
         
         for j in range (0, self.m): #Itterate over #DPTS entires
            found = True
            for k in range(0, i+1): #Itterate over deltas in DPTS entire k -> 0,1,2,3 : 0,1,2 : 0,1 : 0
               
               #i is the table number, there are i+1 entries
               
               #n = 4
               #list 0: ib = 0, i = 3, k = [0],       offset_k = [3]
               #list 1: ib = 1, i = 2, k = [0,1]      offset_k = [2, 3]
               #list 2: ib = 2, i = 1, k = [0,1,2]    offset_k = [1, 2, 3]
               #list 3: ib = 3, i = 0, k = [0,1,2,3]  offset_k = [0, 1, 2, 3]
               
               #print(f"i: {i}, ib: {ib}, j: {j}, k: {k}, offset_k: {k+ib}")
               #print(self.DPTs[i][j].delatas)
               print(f"entry deltas: {self.DPTs[i][j].delatas}, next: {self.DPTs[i][j].prediction}")
               #print(f"comp: {self.DPTs[i][j].delatas[k+ib]} and {delta_seq[k+ib]}")
               if (self.DPTs[i][j].delatas[k+ib] != delta_seq[k+ib]):
                  found = False
                  
            if found: 
               print("found a match!")
               finds.append((i, j)) 
               break  
            
         print("///")    
      
      #all delta seqences that match are now in the found list
      
      if (len(finds) == 0):
         return False, 0
      
      self.__uppdate_LRU(finds)
      
      i, j = finds[0]
      return True, self.DPTs[i][j].prediction
      
            
   def prefetch(self, adress):
      #Block first accses
      if (self.last_adress is None):
         self.last_adress = adress
         return False, 0
      
      new_delta = adress - self.last_adress
      new_delta_seq = [0] * self.n
      
      for i in range (0, self.n-1):
         new_delta_seq[i] = self.last_deltas[i+1]
   
      new_delta_seq[self.n-1] = new_delta
      
      print(f"last deltas: {new_delta_seq}")
      print(f"old deltas:  {self.last_deltas }")
      
      self.__update_deltas(self.last_deltas, new_delta)
      
      valid, next_delta  = self.__find_next_delta(new_delta_seq)
      
      self.last_deltas = new_delta_seq
      self.last_adress =  adress
      
      if (not valid or next_delta is None):
         print("Nothing to prefetch, do dsiplacment insted!")
         return False, 0
      return True, next_delta + adress
   
if __name__ == '__main__':
   prefetcher = Prefetcher(4, 8)
   found, next_delta = prefetcher.prefetch(1)
   found, next_delta = prefetcher.prefetch(2)
   found, next_delta = prefetcher.prefetch(3)
   
   found, next_delta = prefetcher.prefetch(100)
   found, next_delta = prefetcher.prefetch(110)
   found, next_delta = prefetcher.prefetch(120)
   
   found, next_delta = prefetcher.prefetch(1)
   found, next_delta = prefetcher.prefetch(2)
   if found:
      print(next_delta)
