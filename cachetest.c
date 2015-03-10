/*
  * cachetest.c -- Test implementation of multithreaded file cache
  *
  * The disk has NBLOCKS of data; the cache stores many fewer.
  * Our solution assumes a chae of size CACHESIZE.
  */

#include <stdlib.h>
#include <stdio.h>
#include "sthread.h"
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define NTHREADS 10
#define NTESTS 10
#define NBLOCKS 100
#define BLOCKSIZE sizeof(int)

static void tester(int n);
static void cacheinit();
static void readblock(char *, int);
static void writeblock(char *, int);

/* the data being stored and fetched */
static char blockData[NBLOCKS][BLOCKSIZE];

/* randomblock 
 * Generate a random block # from 0..NBLOCKS-1, according to a zipf 
 * distribution, using the rejection method.  The C library random() gives
  * us a uniform distribution, and we discard each option with probability 
  * 1-1/blocknum
  */

int randomblock() {
  int candidate;

  for (;;) {
    candidate = rand() % NBLOCKS;
    if ((double) rand()/RAND_MAX < (double) 1/(candidate + 1))
        return candidate;
    }
}

/* read/write 100 blocks, randomly distributed */
void tester(int n)
{
  int i, blocknum;
  char block[BLOCKSIZE];

  for (i = 0; i < NTESTS; i++) {
    blocknum = randomblock();
    if (rand() % 2) { /* if odd, simulate a write */
      *(int *)block = n * NBLOCKS + blocknum;
      writeblock(block, blocknum); /* write the new value */
      printf("Wrote block %2d in thread %d: %3d\n", blocknum, n, *(int *)block);
    }
    else { /* if even, simulate a read */
      readblock(block, blocknum); 
      printf("Read  block %2d in thread %d: %3d\n", blocknum, n, *(int *)block);
    }
  }
  sthread_exit(100 + n);
  // Not reached
}

int main(int argc, char **argv)
{
  int i; 
  long ret; 
  sthread_t testers[NTHREADS];

  srand(0);	/* init the workload generator */
  cacheinit();  /* init the buffer */

  /* init blocks */
  for (i = 0; i < NBLOCKS; i++) {
    memcpy(blockData[i], (char *) &i, BLOCKSIZE);
  }

  /* start the testers */
  for(i = 0; i < NTHREADS; i++){
    sthread_create(&(testers[i]), &tester, i);
  }
/* wait for everyone to finish */
  for(i = 0; i < NTHREADS; i++){
    ret = sthread_join(testers[i]);
  }
  printf("Main thread done.\n");
  return ret;
}

/* simulated disk block routines
 * simulate out of order completion by the disk 
 * by sleeping for up to 100us */
void dblockread(char *block, int blocknum) {
  // copy from disk[blocknum] to block
  memcpy(block, blockData[blocknum], BLOCKSIZE);
  sthread_sleep(0, rand() % 100000); 
}
void dblockwrite(char *block, int blocknum) {
  // copy from block into disk[blocknum]
  memcpy(blockData[blocknum], block, BLOCKSIZE);
  sthread_sleep(0, rand() % 100000); 
}

/* cache routines */

#define INVALID -1	// the blocknum of empty cache blocks
#define CACHESIZE 4 // cache size

struct cacheBlock {
  // a single block of cache
  smutex_t mutex; // mutex for this block
  int blocknum; // blocknumber of this block
  bool dirty; // whether this block is dirty
  char block[BLOCKSIZE]; // the actual data of this block
};

static struct cacheBlock cache[CACHESIZE];
// defining the cache
// the cache is an array of CACHESIZE cacheBlocks

static int orderArray[CACHESIZE];
// holds indices of blocks in cacheBlock
// when a block needs to be put in, it replaces block at index at front of this
// when a block is initialized/reused, its index is put at the end of orderArray
static smutex_t orderMutex;
// mutex to make sure orderArray reassignment is atomic

// Reshuffles the orderArray
void putToEnd(int indexTemp) {
  // indexTemp is the index in orderArray that needs to be put to end of it
  // notice that indexTemp refers to *contents* of orderArray, not its indices
  
  printf("Put to back: %d\t", indexTemp);

  smutex_lock(&orderMutex); // lock the orderArray
  int startPosition = 0; // from which place up do we reshuffle

  int i;
  for (i = 0; i < CACHESIZE; i++) { // look through the orderArray
    if (orderArray[i] == indexTemp) { // find indexTemp in orderArray
      startPosition = i; // this is the first place to reshuffle
    }
  }

  int j;
  for (j = startPosition; j < (CACHESIZE-1); j++) { // reshuffling
    orderArray[j] = orderArray[j+1]; // move things up
  }
  orderArray[CACHESIZE-1] = indexTemp; // put indexTemp at the end
  
  int k;
  for (k = 0; k < CACHESIZE; k++) {
    printf("Array[%d]: %d\t", k, orderArray[k]);
  }
  printf("\n");

  smutex_unlock(&orderMutex); // unlock orderArray
}

// Initializes the cache
void cacheinit() {
  smutex_init(&orderMutex); // lock orderArray
  
  int i;
  for (i = 0; i < CACHESIZE; i++ ) { // initialize all cacheBlocks
    smutex_init(&cache[i].mutex);
    cache[i].dirty = false;
    cache[i].blocknum = INVALID;
  }
  
  int j;
  for (j = 0; j < CACHESIZE; j++) { // initialize orderArray with 0-CACHESIZE
    // needs to be this way because we initially, we allocate stuff in order
    orderArray[j] = j;
  }
}

// Reads a block
void readblock(char *block, int blocknum) {
  // block provided by tester
  // blocknum is the number of the block to read

  int cacheFound = -1; // where is the block with correct blocknum in cache
  int indexToReplace = 0; // which index do we replace?

  int i;
  for (i = 0; i < CACHESIZE; i++) {
    if (cache[i].blocknum == blocknum) {
      cacheFound = i; // record where we found the correct block
      break;
    }
  }

  if (cacheFound == -1) { // if we did not find the block in cache
    indexToReplace = orderArray[0]; // replacing cacheBlock[head of orderArray]
    smutex_lock(&cache[indexToReplace].mutex); // locks the current cacheBlock
    
    putToEnd(indexToReplace); // update the orderArray
    
    if (cache[indexToReplace].dirty) {
      // we have to write back the contents of previously cached block
      dblockwrite(cache[indexToReplace].block, cache[indexToReplace].blocknum);
    }
    
    dblockread(cache[indexToReplace].block, blocknum); // read blocknum
    cache[indexToReplace].dirty = false; // cacheBlock is clean now
    
    memcpy(block, cache[indexToReplace].block, BLOCKSIZE); // copy to tester
    
    int x;
    for (x = 0; x < CACHESIZE; x++) {
      printf("Cache[%d]: %d\t", x, cache[x].blocknum);
    }
    printf("\n");

    smutex_unlock(&cache[indexToReplace].mutex); // unlocks current cacheBlock
  }

  else { // we found block in cache
    indexToReplace = cacheFound;
    smutex_lock(&cache[indexToReplace].mutex); // locks the cacheBlock
    
    putToEnd(indexToReplace); // update the orderArray
    
    memcpy(block, cache[indexToReplace].block, BLOCKSIZE); // copy to tester
    
    int x;
    for (x = 0; x < CACHESIZE; x++) {
      printf("Cache[%d]: %d\t", x, cache[x].blocknum);
    }
    printf("\n");

    smutex_unlock(&cache[indexToReplace].mutex); // unlocks the cacheBlock
  }
}

void writeblock(char *block, int blocknum) {
  // block provided by tester
  // blocknum is the number of the block to read

  int cacheFound = -1; // where is the block with correct blocknum in cache
  int indexToReplace = 0; // which index do we replace?

  int i;
  for (i = 0; i < CACHESIZE; i++) {
    if (cache[i].blocknum == blocknum) {
      cacheFound = i; // record where we found the correct block
      break;
    }
  }

  if (cacheFound == -1) { // if we did not find the block in cache
    indexToReplace = orderArray[0]; // replacing cacheBlock[head of orderArray]
    smutex_lock(&cache[indexToReplace].mutex); // locks the current cacheBlock
    
    putToEnd(indexToReplace); // update the orderArray
    
    if (cache[indexToReplace].dirty) {
      // we have to write back the contents of previously cached block
      dblockwrite(cache[indexToReplace].block, cache[indexToReplace].blocknum);
    }
    
    cache[indexToReplace].blocknum = blocknum; // rewrite blocknum
    cache[indexToReplace].dirty = true; // make cacheBlock dirty
    memcpy(cache[indexToReplace].block, block, BLOCKSIZE); // copy from tester
    
    int x;
    for (x = 0; x < CACHESIZE; x++) {
      printf("Cache[%d]: %d\t", x, cache[x].blocknum);
    }
    printf("\n");

    smutex_unlock(&cache[indexToReplace].mutex); // unlock current cacheBlock
  }

  else { // we found block in cache
    indexToReplace = cacheFound;
    smutex_lock(&cache[indexToReplace].mutex); // locks the cacheBlock
    
    putToEnd(indexToReplace); // update the orderArray
    
    cache[indexToReplace].blocknum = blocknum; // replace cacheBlock's blocknum
    cache[indexToReplace].dirty = true; // make cacheBlock dirty
    memcpy(cache[indexToReplace].block, block, BLOCKSIZE); // copy from tester
    
    int x;
    for (x = 0; x < CACHESIZE; x++) {
      printf("Cache[%d]: %d\t", x, cache[x].blocknum);
    }
    printf("\n");

    smutex_unlock(&cache[indexToReplace].mutex); // unlock the cacheBlock
  }
}