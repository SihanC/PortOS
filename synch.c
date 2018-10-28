#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "synch.h"
#include "queue.h"
#include "interrupts.h"
#include "minithread.h"

extern minithread_t *curr_thread;

struct semaphore {
    int cnt;
    queue_t *w_queue;
};

semaphore_t* semaphore_create() {
    semaphore_t *s;
    if ((s = (semaphore_t *) malloc(sizeof(semaphore_t))) == NULL) {
        fprintf(stderr, "Failed to allocate\n");
        return NULL;
    }
    s -> w_queue = queue_new();
    if (s -> w_queue == NULL) {
        free(s);
        fprintf(stderr, "Failed to allocate\n");
        return NULL;
    }
    return s;
}

void semaphore_destroy(semaphore_t *sem) {
    if (sem == NULL) 
        return;
    queue_free(sem -> w_queue);
    free(sem);
}

void semaphore_initialize(semaphore_t *sem, int cnt) {
    if (sem == NULL) 
        return;
    sem -> cnt = cnt;
}

void semaphore_P(semaphore_t *sem) {
    if (sem == NULL) 
        return;

    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (sem -> cnt > 0) {
        sem -> cnt--;
    } else {
        queue_append(sem -> w_queue, curr_thread);
        minithread_stop();
    }   
    set_interrupt_level(old_level);
}

void semaphore_V(semaphore_t *sem) {
    if (sem == NULL) 
        return;
    
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (queue_length(sem -> w_queue) == 0) { 
        sem -> cnt++;
    } else {
        minithread_t *next_thread;
        // no thread to switch to
        if (queue_dequeue(sem -> w_queue, (void **) &next_thread) == -1) {
            fprintf(stderr, "Failed to fetch next job from the ready list\n");
            return; 
        }
        minithread_start(next_thread);
    }
    set_interrupt_level(old_level);
}
