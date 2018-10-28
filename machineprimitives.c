/*
 *
 * This file implements some basic primitives, e.g. for manipulating stacks,
 * and performing atomic operations, to be used by the threads package,
 * scheduler, and semaphore implementations.
 *
 * YOU SHOULD NOT [NEED TO] MODIFY THIS FILE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "defs.h"
#include "minithread.h"
#include "machineprimitives.h"

typedef struct initial_stack_state *initial_stack_state_t;

struct initial_stack_state
{
  void *body_proc;            /* v1 or ebx */
  void *body_arg;             /* v2 or edi */
  void *finally_proc;         /* v3 or esi */
  void *finally_arg;          /* v4 or ebp */
  void *r8;
  void *r9;
  void *r10;
  void *r11;
  void *r12;
  void *r13;
  void *r14;
  void *r15;
  void *rax;
  void *rcx;
  void *rdx;
  void *root_proc;            /* left on stack */
};

static const int STACK_GROWS_DOWN      = 1;
static const int STACKSIZE             = (256 * 1024);
static const int STACKALIGN            = 0xf;

void
minithread_allocate_stack(stack_pointer_t *stackbase, stack_pointer_t *stacktop)
{
    *stackbase = (stack_pointer_t) malloc(STACKSIZE);
    if (!*stackbase)  {
        return;
    }

    if (STACK_GROWS_DOWN)
      /* Stacks grow down, but malloc grows up. Compensate and word align
         (turn off low 2 bits by anding with ~3). */
      *stacktop = (stack_pointer_t) ((long)((char*)*stackbase + STACKSIZE - 1) & ~STACKALIGN);
    else {
      /* Word align (turn off low 2 bits by anding with ~3) */
      *stacktop = (stack_pointer_t)(((long)*stackbase + 3)&~STACKALIGN);
    }
}

void
minithread_free_stack(stack_pointer_t stackbase)
{
    free(stackbase);
}

/*
 * See the architecture assembly file.
 */
extern int minithread_root();

void
minithread_initialize_stack(
    stack_pointer_t *stacktop,
    proc_t body_proc,
    arg_t body_arg,
    proc_t finally_proc,
    arg_t finally_arg)
{
    initial_stack_state_t ss;

    /*
     * Configure initial machine state so that this thread starts
     * running inside a wrapper procedure named minithread_root.
     * minithread_root will invoke the procedures in order, and
     * then halt.
     */
    *((char **) stacktop) -= sizeof (struct initial_stack_state);
    ss = (initial_stack_state_t) *stacktop;

    ss->body_proc = (void *) body_proc;
    ss->body_arg = (void *) body_arg;
    ss->finally_proc = (void *) finally_proc;
    ss->finally_arg = (void *) finally_arg;

    ss->root_proc = (void *) minithread_root;
}
