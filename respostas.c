#include "SimpleCache.h"

unsigned char DRAM[DRAM_SIZE];
unsigned int time;
SimpleCache cache; 

/**************** Print Functions ***************/

void printL1() {
  printf("\nL1:\n");
  for (int i = 0; i < (L1_SIZE / BLOCK_SIZE); i++) {
    printf("Index %d: Valid %d; Dirty %d; Tag %d\n", i, cache.L1.Lines[i].Valid, cache.L1.Lines[i].Dirty, cache.L1.Lines[i].Tag);
    for (int j = 0; j < BLOCK_SIZE; j+=WORD_SIZE) {
      unsigned char data[WORD_SIZE];
      memcpy(data, &(cache.L1.Lines[i].Data[j]), WORD_SIZE);
      printf("%d ", *((unsigned int *)data));
    }
    printf("\n");
  }
}

void printL2() {
  printf("\nL2:\n");
  for (int i = 0; i < (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)); i++) {
    for (int j = 0; j < ASSOCIATIVITY_L2; j++) {
      printf("Index %d; Way %d: Valid %d; Dirty %d; Tag %d\n", i, j, cache.L2.Lines[i][j].Valid, cache.L2.Lines[i][j].Dirty, cache.L2.Lines[i][j].Tag);
      for (int k = 0; k < BLOCK_SIZE; k+=WORD_SIZE) {
        unsigned char data[WORD_SIZE];
        memcpy(data, &(cache.L2.Lines[i][j].Data[k]), WORD_SIZE);
        printf("%d ", *((unsigned int *)data));
      }
      printf("\n");
    }
  }
}

void printDRAM() {
  printf("\nDRAM:\n");
  for (int i = 0; i < DRAM_SIZE; i+=WORD_SIZE) {
    unsigned char data[WORD_SIZE];
    memcpy(data, &(DRAM[i]), WORD_SIZE);
    printf("%d ", *((unsigned int *)data));
  }
  printf("\n");
}

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

unsigned int getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(int address, unsigned char *data, int mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() {

  for (int i = 0; i < (L1_SIZE / BLOCK_SIZE); i++) {
    cache.L1.Lines[i].Valid = 0;
    cache.L1.Lines[i].Dirty = 0;
    cache.L1.Lines[i].Tag = 0;
    for (int j = 0; j < BLOCK_SIZE; j+=WORD_SIZE) {
      cache.L1.Lines[i].Data[j] = 0;
    }
  }

  for (int i = 0; i < (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)); i++) {
    for (int j = 0; j < ASSOCIATIVITY_L2; j++) {
      cache.L2.Lines[i][j].Valid = 0;
      cache.L2.Lines[i][j].Dirty = 0;
      cache.L2.Lines[i][j].Tag = 0;
      cache.L2.Lines[i][j].Time = 0;
      for (int k = 0; k < BLOCK_SIZE; k+=WORD_SIZE) {
        cache.L2.Lines[i][j].Data[k] = 0;
      }
    }
  }

  for (int i = 0; i < DRAM_SIZE; i+=WORD_SIZE) {
    DRAM[i] = 0;
  }

}

void accessL2(int address, unsigned char *data, int mode) {

  unsigned int tag, index_n, offset;
  
  tag = address / ((L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)) * BLOCK_SIZE);
  index_n = (address / BLOCK_SIZE) % (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2));
  offset = address % BLOCK_SIZE;

  int found = 0;
  int i = 0;
  while (i < ASSOCIATIVITY_L2 && !found) {
    if (cache.L2.Lines[index_n][i].Valid && cache.L2.Lines[index_n][i].Tag == tag) {
      found = 1;
    } else {
      i++;
    }
  }

  if (!found) {
    i = 0;
    while (i < ASSOCIATIVITY_L2 && cache.L2.Lines[index_n][i].Valid) {
      i++;
    }
    if (i == ASSOCIATIVITY_L2) {
      i = 0;
      unsigned int min = cache.L2.Lines[index_n][0].Time;
      for (int j = 1; j < ASSOCIATIVITY_L2; j++) {
        if (cache.L2.Lines[index_n][j].Time < min) {
          min = cache.L2.Lines[index_n][j].Time;
          i = j;
        }
      }
    }
    if (cache.L2.Lines[index_n][i].Dirty) {
      accessDRAM(cache.L2.Lines[index_n][i].Tag * (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)) * BLOCK_SIZE + index_n * BLOCK_SIZE, cache.L2.Lines[index_n][i].Data, MODE_WRITE);
      cache.L2.Lines[index_n][i].Data[0] = 0;
      cache.L2.Lines[index_n][i].Data[WORD_SIZE] = 0;
    }

    accessDRAM(address - offset, cache.L2.Lines[index_n][i].Data, MODE_READ);

    cache.L2.Lines[index_n][i].Valid = 1;
    cache.L2.Lines[index_n][i].Dirty = 0;
    cache.L2.Lines[index_n][i].Tag = tag;
    cache.L2.Lines[index_n][i].Time = time;

    if (mode == MODE_READ) {
      memcpy(data, &(cache.L2.Lines[index_n][i].Data), BLOCK_SIZE);
      time += L2_READ_TIME;
    }
  } else {
    if (mode == MODE_READ) {
      memcpy(data, &(cache.L2.Lines[index_n][i].Data), BLOCK_SIZE);
      time += L1_READ_TIME;
      cache.L2.Lines[index_n][i].Time = time;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L2.Lines[index_n][i].Data), data, BLOCK_SIZE);
      time += L1_WRITE_TIME;
      cache.L2.Lines[index_n][i].Dirty = 1;
      cache.L2.Lines[index_n][i].Time = time;
    }
  }
  
}

void accessL1(int address, unsigned char *data, int mode) {

  unsigned int tag, index_n, offset;

  tag = address / ((L1_SIZE / BLOCK_SIZE) * BLOCK_SIZE);
  index_n = (address / BLOCK_SIZE) % (L1_SIZE / BLOCK_SIZE);
  offset = address % BLOCK_SIZE;

  if (cache.L1.Lines[index_n].Valid && cache.L1.Lines[index_n].Tag == tag) {

    if (mode == MODE_READ) {
      memcpy(data, &(cache.L1.Lines[index_n].Data[offset]), WORD_SIZE);
      time += L1_READ_TIME;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L1.Lines[index_n].Data[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.L1.Lines[index_n].Dirty = 1;
    }
  }
  else {
    if (cache.L1.Lines[index_n].Dirty) {
      accessL2(cache.L1.Lines[index_n].Tag * (L1_SIZE / BLOCK_SIZE) * BLOCK_SIZE + index_n * BLOCK_SIZE, cache.L1.Lines[index_n].Data, MODE_WRITE);
      cache.L1.Lines[index_n].Data[0] = 0;
      cache.L1.Lines[index_n].Data[WORD_SIZE] = 0;
    }

    accessL2(address - offset, cache.L1.Lines[index_n].Data, MODE_READ);
    if (mode == MODE_READ) {
      memcpy(data, &(cache.L1.Lines[index_n].Data[offset]), WORD_SIZE);
      time += L1_READ_TIME;
      cache.L1.Lines[index_n].Dirty = 0; 
      cache.L1.Lines[index_n].Valid = 1;
      cache.L1.Lines[index_n].Tag = tag;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L1.Lines[index_n].Data[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.L1.Lines[index_n].Dirty = 1;
      cache.L1.Lines[index_n].Valid = 1;
      cache.L1.Lines[index_n].Tag = tag;
    }
  }
}

void read(int address, unsigned char *data) {
  accessL1(address, data, MODE_READ);
}

void write(int address, unsigned char *data) {
  accessL1(address, data, MODE_WRITE);
}
