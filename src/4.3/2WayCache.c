#include "2WayCache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache cache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

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

void initCache() { 
  cache.L1.init = 1;
  for (int i = 0; i < L1_LINES; i++){
    cache.L1.line[i].Valid = 0;
    cache.L1.line[i].Dirty = 0;
    cache.L1.line[i].Tag = 0;
    for (int j = 0; j < BLOCK_SIZE; j+=WORD_SIZE) {
      cache.L1.line[i].block[j] = 0;
    }
  }

  cache.L2.init = 1;
  for (int i = 0; i < L2_LINES/2; i++){
    for (int k = 0; k < ASSOC; k++){
      cache.L2.line[i][k].Valid = 0;
      cache.L2.line[i][k].Dirty = 0;
      cache.L2.line[i][k].Tag = 0;
      for (int j = 0; j < BLOCK_SIZE; j+=WORD_SIZE) {
        cache.L2.line[i][k].block[j] = 0;
      }
    }
  }
}

void accessL2(uint32_t address, uint8_t *data, uint32_t mode){
  uint32_t index, Tag, MemAddress, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  Tag = address >> 14;
  MemAddress = address;
  index = MemAddress % L2_LINES;
  offset = address % BLOCK_SIZE; 

  int set = 0;
  for (int i = 0; i < ASSOC; i++){
    if (cache.L2.line[index][i].Valid && cache.L2.line[index][i].Tag == Tag){
      set = i;
      break;
    }
  }

  CacheLine *Line = &cache.L2.line[index][set];

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    set = 0;
    for (int i = 0; i < ASSOC && cache.L2.line[index][i].Valid; i++){
      set++;
    }
    if (set == ASSOC){
      set = 0;
      unsigned int time = cache.L2.line[index][0].time;
      for (int i = 1; i < ASSOC; i++){
        if (cache.L2.line[index][i].time < time){
          time = cache.L2.line[index][i].time;
          set = i;
        }
      }
    }
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      accessDRAM(MemAddress, &(cache.L2.line[index][set].block[0]),
                 MODE_WRITE); // then write back old block
    }
    memcpy(&(cache.L2.line[index][set].block[0]), TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {
      memcpy(data, &(cache.L2.line[index][set].block[offset]), BLOCK_SIZE);
      time += L2_READ_TIME;
      Line->time = time;
    }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(cache.L2.line[index][set].block[offset]), data, BLOCK_SIZE); //word size para block size
    time += L2_WRITE_TIME;
    Line->time = time;
    Line->Dirty = 1;
  }
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, MemAddress, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  Tag = address >> 14;
  MemAddress = address >> 6; // remove offset bits
  index = MemAddress % L1_LINES;
  offset = address % BLOCK_SIZE;

  CacheLine *Line = &cache.L1.line[index];

if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessL2(MemAddress, TempBlock, MODE_READ); // get new block from L2

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      accessL2(MemAddress, &(cache.L1.line[index].block[0]),
                 MODE_WRITE); // then write back old block
    }
    memcpy(&(cache.L1.line[index].block[0]), TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block
  
    
  if (mode == MODE_READ) {
    memcpy(data, &(cache.L1.line[index].block[offset]), WORD_SIZE);
    time += L1_READ_TIME;

  }
  if (mode == MODE_WRITE) {
    memcpy(&(cache.L1.line[index].block[offset]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }

}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}