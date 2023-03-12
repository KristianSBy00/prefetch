#define n 4
#define m 8
#define p 64

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