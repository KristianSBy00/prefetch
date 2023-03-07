from dataclasses import dataclass

@dataclass
class DPTEntry:
   next_delta: int
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
            self.DPTs[i].append(DPTEntry(None, [None] * (n+1)))
            
   #This function updates the delata 
   def __update_deltas(self, raw_delta_sequence, next_delta):
      
      #Exmple:
      
      #raw_delata_seqence = 1,2,4,8
      #n = 4
      
      #i=0: delta seqence = 8
      #i=1: delta seqence = 4,8
      #i=2: delta seqence = 2,4,8
      #i=3: delta seqence = 1,2,4,8
      
      for i in range(0, self.n):
         victem = 0
         
         d_i = self.n - i - 1
         
         delta_seqence = []
         
         #n = 4
         # i = 0: b = [1],          append = 3,          diff = 2
         # i = 1: b = [1, 2],       append = 2, 3,       diff = 1 
         # i = 2: b = [1, 2, 3],    append = 1, 2, 3     diff = 0 
         # i = 3: b = [1, 2, 3, 4], append = 0, 1, 2, 3  diff = -1
         
         for b in range (1, i+2):
            delta_seqence.append(raw_delta_sequence[b + self.n -2 - i])
         
         for j in range (1, j):
            if (self.DPTs[i][j].LRU > self.DPTs[i][j].LRU):
               victem = j
               
         self.DPTs[i][victem] = DPTEntry(next_delta, delta_seqence)
                  
   #This function finds the next delta
   def __find_next_delta(self): 
      
      finds = []
      
      for ib in range (self.n-1, -1): #Itterate over DPTs
         i = self.n - ib - 1 #i -> n-1, n-2, n-3 ... 0
         found = True
         
         for j in range (0, self.m): #Itterate over #DPTS entires
            
            #next_delta = self.DPTs[i][j].next_delta
            
            for k in range(0, i+1): #Itterate over deltas in DPTS entire
               
               #i is the table number, there are i+1 entries
               
               #n = 4
               #list 0: ib = 0, i = 3, k = [0],       offset_k = [3]
               #list 1: ib = 1, i = 2, k = [0,1]      offset_k = [2, 3]
               #list 2: ib = 2, i = 1, k = [0,1,2]    offset_k = [1, 2, 3]
               #list 3: ib = 3, i = 0, k = [0,1,2,3]  offset_k = [0, 1, 2, 3]
               
               #offset = self.n-i+1
               
               if (self.DPTs[i][j].delatas[k] != self.last_deltas[k+i]):
                  found = False
                  
            if (found): 
               finds.append((i, j)) 
               break      
      
      #all delta seqences that match are now in the found list
      for i, j in finds:
         for k in range (0, self.m):
            self.DPTs[i][k].LRU += 1
      
            #return True, next_delta
            #self.DPTs[i][j].LRU = 0 #Update Lru
            
         self.DPTs[i][j].LRU = 0
         
      if (len(finds) == 0):
         return False, 0
      
      i, j = finds[0]
      return True, self.DPTs[i][j]
      
            
   def prefetch(self, adress):
      
      if (self.last_adress == None):
         self.last_adress = adress
         return False, 0
      
      new_delta = adress - self.last_adress
      
      tmp = self.last_deltas
      
      print(len(self.last_deltas))
      
      for i in range (0, self.n-1):
         self.last_deltas[i] = tmp[i+1]
   
      self.last_deltas[self.n-1] = new_delta
      
      valid, next_delta  = self.__find_next_delta()
      
      self.__update_deltas(tmp, new_delta)
   
      #Uppdate deltas
      
      if (not valid):
         print("Nothing to prefetch, do dsiplacment insted!")
         return False, 0
      
                   
      self.last_adress =  adress
      return True, next_delta + adress
   
if __name__ == '__main__':
   prefetcher = Prefetcher(4, 64)
   found, next_delta = prefetcher.prefetch(1)
   found, next_delta = prefetcher.prefetch(2)
   found, next_delta = prefetcher.prefetch(3)
   print(next_delta)
