/*
  * cachetest.c -- Test implementation of multithreaded file cache
  *
  * The disk has NBLOCKS of data; the cache stores many fewer.
  * Our stub assumes a cache size of one block, the trivial case.
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
    if (rand() % 2) {	/* if odd, simulate a write */
      *(int *)block = n * NBLOCKS + blocknum;
      writeblock(block, blocknum); /* write the new value */
      printf("Wrote block %2d in thread %d: %3d\n", blocknum, n, *(int *)block);
    }
    else {		/* if even, simulate a read */
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
  memcpy(block, blockData[blocknum], BLOCKSIZE);
  sthread_sleep(0, rand() % 100000); 
}
void dblockwrite(char *block, int blocknum) {
  memcpy(blockData[blocknum], block, BLOCKSIZE);
  sthread_sleep(0, rand() % 100000); 
}

/* stub routines 
 * We've implemented a single item cache in a particularly inefficient fashion.
 */

#define INVALID -1	// cache starts empty

struct blockcache {
  int blocknum;		// blocknumber of the current block in the cache
  bool dirty;		// whether the block is dirty
  char block[BLOCKSIZE]; // storage for the block of data
};

/* mutual exclusion */
static smutex_t mutex;
static struct blockcache cache;

void cacheinit() {
  smutex_init(&mutex);
  cache.dirty = false;
  cache.blocknum = INVALID; 
}

void readblock(char *block, int blocknum) {
  smutex_lock(&mutex);
  if (cache.blocknum != blocknum) {
     if (cache.dirty) { /* we have to write back the cached block */
        dblockwrite(cache.block, cache.blocknum);
     }
     dblockread(cache.block, blocknum);
     cache.dirty = false;
  } /* otherwise its already here! */
  memcpy(block, cache.block, BLOCKSIZE); 
  smutex_unlock(&mutex);
}

void writeblock(char *block, int blocknum) {
  smutex_lock(&mutex);
  if (cache.dirty && cache.blocknum != blocknum) {
    dblockwrite(cache.block, cache.blocknum); /* we have to write it back */
  }
  cache.blocknum = blocknum;
  cache.dirty = true;
  memcpy(cache.block, block, BLOCKSIZE); 
  smutex_unlock(&mutex);
}

