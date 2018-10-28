#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "alarm.h"
#include "interrupts.h"
#include "minithread.h" 
#include "miniheader.h"
#include "minimsg.h"
#include "minisocket.h"
#include "multilevel_queue.h"
#include "network.h" 
#include "queue.h"
#include "synch.h"

/* minithread control block definition */
struct minithread {
    int tid;
    int level;
    int quantum_remain;
    stack_pointer_t stack_ptr;
    stack_pointer_t stack_base;
};

int next_tid = 1;               // next thread id to allocate
int quantum = 160;              // total quantum for 1 cycle
queue_t *f_list;                // finished list
queue_t *alarm_list;            // alarm list
multilevel_queue_t *rd_list;    // ready list
minithread_t *curr_thread;      // current running thread
minithread_t *k_thread;         // kernel thread
minithread_t *cleanup_thread;   // cleanup thread
semaphore_t *cleanup_sem;

/* 
 * x == 1, added back to the queue, x == 0 otherwise
 */
void minithread_schedule(int x) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    minithread_t *next_thread;
    curr_thread -> quantum_remain--;
    quantum--;
    int next_level = curr_thread -> level;
    if (curr_thread -> quantum_remain == 0) {
        // Level 3 will just enqueue at the end of the queue
        if (curr_thread -> level != 3) {
            next_level++;
            curr_thread -> level = next_level;
        }

        // Update quantum to 2^i, where i is level 
        curr_thread -> quantum_remain = 1 << next_level;
    }       

    if (multilevel_queue_length(rd_list) == 0) {
        if (quantum == 0)
            quantum = 160;

        // Switch to kernel thread
        minithread_switch(&(curr_thread -> stack_ptr), &(k_thread -> stack_ptr));
        return;
    }

    if (quantum > 80) {           // level 0
        multilevel_queue_dequeue(rd_list, 0, (void **) &next_thread);
    } else if (quantum > 40) {    // level 1
        multilevel_queue_dequeue(rd_list, 1, (void **) &next_thread);
    } else if (quantum > 16) {    // level 2
        multilevel_queue_dequeue(rd_list, 2, (void **) &next_thread);
    } else if (quantum > 0) {     // level 3
        multilevel_queue_dequeue(rd_list, 3, (void **) &next_thread);
    } else if (quantum == 0) {    // switch back to level 0
        quantum = 160;
        multilevel_queue_dequeue(rd_list, 0, (void **) &next_thread);
    }
    // Need to set current thread to next at here, since thread will switch out 
    minithread_t *tmp_thread = curr_thread; 
    curr_thread = next_thread;   

    // Not calling from minithread_stop, so add back to the ready list
    if (x == 1 && tmp_thread != k_thread) {
        multilevel_queue_enqueue(rd_list, next_level, tmp_thread);
    }
    minithread_switch(&(tmp_thread -> stack_ptr), &(curr_thread -> stack_ptr));
    
    set_interrupt_level(old_level);
}

/* Final bookkeeping function */
int finalProc(int *argv) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (queue_append(f_list, curr_thread) == -1) {
        fprintf(stderr, "Failed to append to the finished queue.");
        exit(0);
    }
    semaphore_V(cleanup_sem);
    // Don't add the thread back to the ready queue
    minithread_schedule(0);
    set_interrupt_level(old_level);
    exit(0);
}

/* Helper function to clean up thread */
void clean_up_helper(void *t, void *x) {
    minithread_t *f_thread;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (queue_dequeue(f_list, ( void ** )&f_thread ) == -1) {       // no thread to switch to
        fprintf(stderr, "Failed to fetch next job from the final list\n");
        return; 
    }
    set_interrupt_level(old_level);
    minithread_free_stack(f_thread -> stack_base);
    free(f_thread);
    return;
} 

/* Cleap up thread routine */
int clean_up(int *x) {
    while (1) {
        semaphore_P(cleanup_sem);
        interrupt_level_t old_level = set_interrupt_level(DISABLED);
        if (queue_length(f_list) > 0)
            queue_iterate(f_list, &clean_up_helper, NULL);
        set_interrupt_level(old_level);
        minithread_yield();
    }
}

/* minithread functions */
minithread_t* minithread_fork(proc_t proc, arg_t arg) {
    // create a thread
    minithread_t *new_thread;
    if ((new_thread = minithread_create(proc, arg)) == NULL) {
        fprintf(stderr, "Failed to creating new thread.");
        return NULL;
    }
    // Add new job to the ready list.
    minithread_start(new_thread);
    return new_thread;
}

minithread_t* minithread_create(proc_t proc, arg_t arg) {
    // Create a new thread
    minithread_t *thread_ptr = (minithread_t *) malloc(sizeof(minithread_t));
    if (thread_ptr == NULL) {
        fprintf (stderr, "Allocate thread_ptr failed\n");
        return NULL;
    }
    
    minithread_allocate_stack(&(thread_ptr -> stack_base), 
        &(thread_ptr -> stack_ptr));
    minithread_initialize_stack(&(thread_ptr -> stack_ptr), 
        proc, arg, finalProc, NULL);
    // allocate stack failed
    if (thread_ptr -> stack_ptr == NULL) {
        free(thread_ptr);
        fprintf(stderr, "Allocate stack failed\n");
        return NULL;
    }
    thread_ptr -> level = 0;
    thread_ptr -> quantum_remain = 1;
    // Disable interrupt to prevent two thread have the same tid.
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    thread_ptr -> tid = next_tid;
    next_tid++;
    set_interrupt_level(old_level);
    return thread_ptr;
}

minithread_t* minithread_self() {
    return curr_thread;
}

int minithread_id() {
    return curr_thread -> tid;
}

void minithread_stop() {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    minithread_schedule(0);
    set_interrupt_level(old_level);
}

void minithread_start(minithread_t *t) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (t == NULL) 
        return;
    // Thread that is made runnable is enqueued to level 0
    if (multilevel_queue_enqueue(rd_list, 0, (void*) t) == -1) {
        fprintf(stderr, "Failed to start the thread\n");
        return;
    }
    set_interrupt_level(old_level);
}

void minithread_yield() {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (multilevel_queue_length(rd_list) > 0) {
        // If queue length is 1, it will yield to itself
        minithread_schedule(1);   
    }
    set_interrupt_level(old_level);
}

/*
 * This is the clock interrupt handler.
 * Inside minithread_system_initialize you should call minithread_clock_init
 * with this function as a parameter.
 */
void clock_handler(void* arg) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    ticks++;
    do_alarms();
    if (curr_thread != k_thread) {
        minithread_yield(); 
    } else if (curr_thread == k_thread && 
        multilevel_queue_length(rd_list) > 1) {
        // Have thread other than cleanup thread in the ready list
        // kernel thread should not be in the ready list.
        minithread_schedule(0);   
    }
    set_interrupt_level(old_level); 
}

void network_handler(network_interrupt_arg_t* arg) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    mini_header_t *header = (mini_header_t *) arg -> buffer;
    if (header -> protocol == PROTOCOL_MINIDATAGRAM) {
        minimsg_append(arg);
    } else if (header -> protocol == PROTOCOL_MINISTREAM) {
        minisocket_append(arg);
    }
    set_interrupt_level(old_level);
}

void minithread_sleep_with_timeout(int delay) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    minithread_t *tmp_thread = curr_thread;
    alarm_id ret = alarm_register(delay / 1000, 
        (alarm_handler_t) minithread_start, tmp_thread);
    assert(ret != NULL);
    set_interrupt_level(old_level);
    minithread_stop();
}

void minithread_system_initialize(proc_t mainproc, arg_t mainarg) { 
    int ret; 

    // Initialize ready list and finish list 
    rd_list = multilevel_queue_new(4);
    f_list = queue_new();
    alarm_list = queue_new();

    // Initialize kernel thread
    k_thread = (minithread_t *) malloc(sizeof(minithread_t));   
    if (k_thread == NULL) {
        fprintf(stderr, "Failed to initialize kernel thread.");
        return;
    }
    // Since k_thread will not be in the ready list
    // don't need to initialize level and quantum_remain
    k_thread -> stack_ptr = NULL;
    k_thread -> stack_base = NULL;
    k_thread -> tid = next_tid;
    next_tid++;
    curr_thread = k_thread;

    minithread_t *c_thread; 
    cleanup_sem = semaphore_create();
    semaphore_initialize(cleanup_sem, 0);
    
    // Kernel create the first thread or Kernel create the reaper thread    
    if (minithread_fork(mainproc, mainarg) == NULL ||      
        (c_thread = minithread_fork(&clean_up, NULL)) == NULL) {
        fprintf(stderr, "Failed to create new thread\n");   
        return;
    }
    cleanup_thread = c_thread;

    // Initialize interrupt
    minithread_clock_init(&clock_handler);
    ret = network_initialize(&network_handler);
    assert(ret == 0);
    minimsg_initialize();
    minisocket_initialize();
    set_interrupt_level(ENABLED);

    // Idle loop
    while (1) {
        interrupt_level_t old_level = set_interrupt_level(DISABLED);
        // Not just the clean-up thread in the queue
        if (multilevel_queue_length(rd_list) > 0) {   
            minithread_yield();
            curr_thread = k_thread;
        }
        set_interrupt_level(old_level);
    }

    free(k_thread);
}