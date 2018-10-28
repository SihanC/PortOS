/*
 * Definitions for minithreads.
 */

#ifndef __MINITHREAD_H__
#define __MINITHREAD_H__

#include "queue.h"
#include "machineprimitives.h"

/*
 * struct minithread:
 *  This is the key data structure for the thread management package.
 *  You must define the thread control block as a struct minithread.
 */
typedef struct minithread minithread_t;

/* Global variables needed in other files*/
extern queue_t *alarm_list;		// list of waiting alarms
extern int current_time;		// time in milliseconds

/*
 * Create and schedule a new thread of control so
 * that it starts executing inside proc_t with
 * initial argument arg.
 */
minithread_t* minithread_fork(proc_t proc, arg_t arg);

/*
 * Like minithread_fork, only returned thread is not scheduled
 * for execution.
 */
minithread_t* minithread_create(proc_t proc, arg_t arg);

/*
 * Return identity (minithread_t) of caller thread.
 */
minithread_t* minithread_self();

/*
 * Return thread identifier of caller thread, for debugging.
 */
int minithread_id();

/*
 * Block the calling thread.
 */
void minithread_stop();

/*
 * Make t runnable.
 */
void minithread_start(minithread_t *t);

/*
 * Forces the caller to relinquish the processor and be put to the end of
 * the ready queue.  Allows another thread to run.
 */
void minithread_yield();

/*
 * Initialize the system to run the first minithread at
 * mainproc(mainarg).  This procedure should be called from your
 * main program with the callback procedure and argument specified
 * as arguments.
 */
void minithread_system_initialize(proc_t mainproc, arg_t mainarg);

/*
 * sleep with timeout in microseconds
 */
void minithread_sleep_with_timeout(int delay);


#endif /*__MINITHREAD_H__*/

