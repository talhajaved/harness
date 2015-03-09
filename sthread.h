#ifndef _STHREAD_H_
#define _STHREAD_H_

#ifdef __cplusplus
extern "C"{
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif

/*
 Note: this library requires you to link with the posix threads
 library (-lpthread) and the real time library (-lrt) {for
 nanosleep}.

   c++ -D_POSIX_PTHREAD_SEMANTICS main.cc sthread.c -lpthread -lrt
   or 
   g++ -Wall -D_POSIX_PTHREAD_SEMANTICS main.cc sthread.c -lpthread -lrt
*/

#include <pthread.h>
#include <unistd.h>

typedef pthread_mutex_t smutex_t;
typedef pthread_cond_t scond_t;
typedef pthread_t sthread_t;

/*
 * API for managing threads
 *
 * First the simplified version where the called
 * function takes an int as an argument and returns
 * an int when it's done.
 *
 * Then the more flexible version where the argument
 * and return are arbitrary pointers.
 */
void sthread_create(sthread_t *thrd,
		    void (*start_routine)(int), 
		    int arg_to_start_routine);
void sthread_yield();
long sthread_join(sthread_t thrd);
void sthread_exit(int ret);

void sthread_create_p(sthread_t *thrd,
                      void *(*start_routine)(void*), 
                      void *arg_to_start_routine);
void *sthread_join_p(sthread_t thrd);
void sthread_exit_p(void *ret);

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
void sthread_sleep(unsigned int seconds, unsigned int nanoseconds);



/*
 * API for mutex locks
 */
void smutex_init(smutex_t *mutex);
void smutex_destroy(smutex_t *mutex);
void smutex_lock(smutex_t *mutex);
void smutex_unlock(smutex_t *mutex);

/*
 * API for condition variables
 */
void scond_init(scond_t *cond);
void scond_destroy(scond_t *cond);

/*
 * Condition variables are always associated with state
 * variables that you access before signalling, broadcasting,
 * or waiting. To access the state variable, you must hold
 * the associated mutex. To help enforce this, you 
 * are required to hold the mutex and pass it in as an
 * argument to these functions.
 */
void scond_signal(scond_t *cond, smutex_t *mutex);
void scond_broadcast(scond_t *cond, smutex_t *mutex);
void scond_wait(scond_t *cond, smutex_t *mutex);



#ifdef __cplusplus
} /* extern C */
#endif

#endif

