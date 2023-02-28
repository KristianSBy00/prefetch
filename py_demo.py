from dataclasses import dataclass

@dataclass
class DPTEntry:
   next_delta: int
   delatas: list
   LRU = 0

class Prefetcher: 
   
   def __init__(self, n, m):
      self.last_adress = None
      self.last_deltas = [n]
      self.DPTs = []
      self.n = n #Number of delta prediction tables
      self.m = m #Number of entries in delta predicton tables
      
      for i in range (0,n):
         self.DPTs.append([])
         for j in range (0,m):
            self.DPTs[i].append(DPTEntry(None, [None] * (n+1)))
            
   def prefetch(self, adress):
      new_dalta = adress - self.last_adress
      
      tmp = self.last_deltas
      
      for i in range (0, self.n):
         self.last_deltas[i] = tmp[i+1]
   
      self.last_deltas[self.n-1] = new_dalta
      
      for ib in range (self.n-1, -1): #Itterate over DPTs
         i = self.n - ib - 1 #i -> n-1, n-2, n-3 ... 0
         found = True
         
         for j in range (0, self.m): #Itterate over #DPTS entires
            
            next_delta = self.DPTs[i][j].next_delta
            
            for k in range(0, i+1): #Itterate over deltas in DPTS entire
               
               #i is the table number, there are i+1 entries
               
               #n = 4
               #list 0: ib = 0, i = 3, k = [0],       offset_k = [3]
               #list 1: ib = 1, i = 2, k = [0,1]      offset_k = [2, 3]
               #list 2: ib = 2, i = 1, k = [0,1,2]    offset_k = [1, 2, 3]
               #list 3: ib = 3, i = 0, k = [0,1,2,3]  offset_k = [0, 1, 2, 3]
               
               offset = self.n-i+1
               
               if (self.DPTs[i][j].delatas[k] != self.last_deltas[k+i]):
                  found = False
              
            if found:
               break
             
         if found:
            break  
                   
      self.last.adress =  adress
      return next_adress 
   
if __name__ == '__main__':
   #prefetcher = Prefetcher(4, 64)
    
