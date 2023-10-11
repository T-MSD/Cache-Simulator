/* The memory hierarchy with L1 and L2 cache works 
by storing frequently accessed data in the smaller 
and faster L1 cache, which is closer to the CPU, 
and less frequently accessed data in the larger 
but slower L2 cache. When the CPU requests data, 
it first checks the L1 cache. If the data is found there, 
it is returned to the CPU. If the data is not found 
in the L1 cache, the CPU checks the L2 cache. I
f the data is found there, it is returned to the CPU 
and also copied to the L1 cache for faster access in the future. 
If the data is not found in either cache, 
it is retrieved from main memory and copied to both caches for future access. 
This process is known as caching and helps 
to reduce the average memory access time and improve system performance. */

#include "L2Cache.h"

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
  for (int i = 0; i < L2_LINES; i++){
    cache.L2.line[i].Valid = 0;
    cache.L2.line[i].Dirty = 0;
    cache.L2.line[i].Tag = 0;
    for (int j = 0; j < BLOCK_SIZE; j+=WORD_SIZE) {
      cache.L2.line[i].block[j] = 0;
    }
  }
}

void accessL2(uint32_t address, uint8_t *data, uint32_t mode){
  uint32_t index, Tag, MemAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  Tag = address;
  MemAddress = address;
  index = MemAddress % L2_LINES;

  CacheLine *Line = &cache.L2.line[index];

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      accessDRAM(MemAddress, &(cache.L2.line[index].block[0]),
                 MODE_WRITE); // then write back old block
    }
    memcpy(&(cache.L2.line[index].block[0]), TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(cache.L2.line[index].block[0]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(cache.L2.line[index].block[0]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, MemAddress, offset;

  Tag = address >> 6; // remove offset bits
  MemAddress = address >> 6; // remove offset bits
  index = MemAddress % L1_LINES;
  offset = address % BLOCK_SIZE;

  CacheLine *Line = &cache.L1.line[index];

  if (Line->Valid && Line->Tag == Tag){
    if (mode == MODE_READ) {
      memcpy(data, &(cache.L1.line[index].block[offset]), WORD_SIZE);
      time += L1_READ_TIME;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L1.line[index].block[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.L1.line[index].Dirty = 1;
    }
  }
  
  else{
    accessL2(address - offset, cache.L1.line[index].block, MODE_READ);
    if (mode == MODE_READ) {
      memcpy(data, &(cache.L1.line[index].block[offset]), WORD_SIZE);
      time += L1_READ_TIME;
      cache.L1.line[index].Dirty = 0; 
      cache.L1.line[index].Valid = 1;
      cache.L1.line[index].Tag = Tag;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L1.line[index].block[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.L1.line[index].Dirty = 1;
      cache.L1.line[index].Valid = 1;
      cache.L1.line[index].Tag = Tag;
    }
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}

/*In the accessL2 function call on line 125, the first argument is 
address - offset. This is because the accessL2 function expects the 
starting address of a cache block, which is the address of the first
 byte in the block. Since the accessL2 function is called when the 
 requested data is not found in the L1 cache, it needs to retrieve 
 the entire cache block from the L2 cache and store it in the 
 corresponding L1 cache line. Therefore, the starting address 
 of the cache block is calculated by subtracting the offset of the 
 requested address within the cache block from the requested address 
 itself. This gives the address of the first byte in the cache block 
 that needs to be retrieved from the L2 cache.*/