#ifndef SIMPLECACHE_H
#define SIMPLECACHE_H
#define L1_LINES (L1_SIZE/BLOCK_SIZE)
#define L2_LINES (L2_SIZE/BLOCK_SIZE)
#define ASSOC 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Cache *************************/

void initCache();
void accessL1(uint32_t, uint8_t *, uint32_t);

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
  uint8_t block[BLOCK_SIZE];
  unsigned int time;
} CacheLine;

typedef struct L1Cache {
  uint32_t init;
  CacheLine line[L1_LINES];
}L1Cache;

typedef struct L2Cache {
  uint32_t init;
  CacheLine line[L2_LINES/2][ASSOC];
}L2Cache;

typedef struct Cache {
  L1Cache L1;
  L2Cache L2;
}Cache;

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
