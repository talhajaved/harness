#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "sthread.h"

/*
 * sthread_create()
 *
 * Create a new thread, that will run a specified routine
 * as its initial function. That function gets a specified 
 * int as an argument.
 *
 *     thread -- data structure to represent the thread
 *     start_routine -- pointer to the function the thread
 *                      should run
 *     argToStartRoutine -- argument to pass to the start_routine
 */
void sthread_create(sthread_t *thrd,
		    void (*start_routine)(int), 
		    int arg_to_start_routine)
{
  //
  // When a detached thread returns from
  // its entry function, the thread will be destroyed.  If we
  // don't detach it, then the memory associated with the thread
  // won't be cleaned up until somebody "joins" with the thread
  // by calling pthread_wait().
  //
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);


  // We are using pthreads which expects to pass a pointer
  // as the argument to the start routine. To make some
  // examples in the book simpler, we pass an int instead.
  // For safety, we need to make sure that an int fits in this
  // pointer argument.
  assert(sizeof(int) <= sizeof(void *));


  if(pthread_create(thrd, 
                    &attr, 
                    (void *(*)(void *))start_routine, 
                    (void *)(intptr_t)arg_to_start_routine)){
      perror("pthread_create failed");
      exit(-1);
  }


}




/*
 * sthread_create_p()
 *
 * Create a new thread, that will run a specified routine
 * as its initial function. That function gets a specified 
 * pointer (to anything) as an argument.
 *
 *     thread -- data structure to represent the thread
 *     start_routine -- pointer to the function the thread
 *                      should run
 *     argToStartRoutine -- argument to pass to the start_routine
 *                          void* means that it can point to anything.
 *                          start_routine() must know what type
 *                          it really is and cast it appropriately.
 */
void sthread_create_p(sthread_t *thread,
                      void *(*start_routine)(void*), 
                      void *argToStartRoutine)
{
  //
  // When a detached thread returns from
  // its entry function, the thread will be destroyed.  If we
  // don't detach it, then the memory associated with the thread
  // won't be cleaned up until somebody "joins" with the thread
  // by calling pthread_join().
  //
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  if(pthread_create(thread, &attr, start_routine, argToStartRoutine)){
      perror("pthread_create failed");
      exit(-1);
  }

}


/*
 * sthread_yield()
 *
 * Volunteer to give up the CPU so that some other thread can run
 * At some future point, the scheduler will reschedule the calling
 * thread and it will return as if it had just called a (slow)
 * null procedure call.
 */
void sthread_yield()
{
  int err = sched_yield();
  assert(err == 0);
  return;
}

/*
 * sthread_join()
 *
 * Wait for the specified thread to call sthread_exit() (if
 * it hasn't already.)
 *
 * The value passed to sthread_exit() by the terminating thread
 * is returned so that the joining
 * thread can receive the "return value" of the terminating
 * thread.
 *
 * We use casting to convert the "pointer to a pointer to
 * void" into a "pointer to int", and we assume that
 * an int fits into a pointer to void. See sthread_exit().
 */
long sthread_join(sthread_t thrd)
{
  long ret;
  void *retv;
  assert(sizeof(sthread_t) == sizeof(pthread_t));
  int err = pthread_join((pthread_t)thrd, &retv);
  assert(err == 0);
  //assert(sizeof(int) >= sizeof(void *));
  ret = (long)retv;
  return ret;
}


/*
 * sthread_join_p()
 *
 * Wait for the specified thread to call sthread_exit_p() (if
 * it hasn't already.)
 *
 * The value passed to sthread_exit() by the terminating thread
 * is returned so that the joining
 * thread can receive the "return value" of the terminating
 * thread.
 *
 */
void *sthread_join_p(sthread_t thrd)
{
  void *ret;
  int err = pthread_join((pthread_t)thrd, &ret);
  assert(err == 0);
  return ret;
}



void sthread_exit(int ret)
{
  /*
   * The underlying pthread_exit returns a pointer
   * so that the thread can hand back a pointer to
   * an arbitrary structure. For simplicity, we're
   * only going to allow thread to pass back an int.
   * To avoid allocating new memory (and causing a 
   * leak), we assume that we can fit an int
   * into a pointer and cast it.
   */
  assert(sizeof(int) <= sizeof(void *));
  pthread_exit((void *)(intptr_t)ret);
}

void sthread_exit_p(void *ret)
{
  pthread_exit(ret);
}




/*
 * WARNING:
 * Do not use sleep for synchronizing threads that 
 * should be waiting for events (using condition variables)!
 * Sleep should only be used to wait for a specified amount
 * of time! (If you find yourself looping on a predicate
 * and calling sleep in the loop, you probably are using
 * it incorrectly! We will deduct points from your grade
 * if you do this!
 */
void sthread_sleep(unsigned int seconds, unsigned int nanoseconds)
{
  struct timespec rqt;
  assert(nanoseconds < 1000000000);
  rqt.tv_sec = seconds;
  rqt.tv_nsec = nanoseconds;
  if(nanosleep(&rqt, NULL) != 0){
    perror("sleep failed. Woke up early");
    exit(-1);
  }
}






void smutex_init(smutex_t *mutex)
{
  if(pthread_mutex_init(mutex, NULL)){
      perror("pthread_mutex_init failed");
      exit(-1);
  }    
}

void smutex_destroy(smutex_t *mutex)
{
  if(pthread_mutex_destroy(mutex)){
      perror("pthread_mutex_destroy failed");
      exit(-1);
  }    
}

void smutex_lock(smutex_t *mutex)
{
  if(pthread_mutex_lock(mutex)){
    perror("pthread_mutex_lock failed");
    exit(-1);
  }    
}

void smutex_unlock(smutex_t *mutex)
{
  if(pthread_mutex_unlock(mutex)){
    perror("pthread_mutex_unlock failed");
    exit(-1);
  }    
}



void scond_init(scond_t *cond)
{
  if(pthread_cond_init(cond, NULL)){
      perror("pthread_cond_init failed");
      exit(-1);
  }
}

void scond_destroy(scond_t *cond)
{
  if(pthread_cond_destroy(cond)){
      perror("pthread_cond_destroy failed");
      exit(-1);
  }
}

void scond_signal(scond_t *cond, smutex_t *muted /* NOTUSED */)
{
  //
  // assert(mutex is held by this thread);
  //

  if(pthread_cond_signal(cond)){
    perror("pthread_cond_signal failed");
    exit(-1);
  }
}

void scond_broadcast(scond_t *cond, smutex_t *mutex /* NOTUSED */)
{
  //
  // assert(mutex is held by this thread);
  //
  if(pthread_cond_broadcast(cond)){
    perror("pthread_cond_broadcast failed");
    exit(-1);
  }
}

void scond_wait(scond_t *cond, smutex_t *mutex)
{
  //
  // assert(mutex is held by this thread);
  //

  if(pthread_cond_wait(cond, mutex)){
    perror("pthread_cond_wait failed");
    exit(-1);
  }
}


