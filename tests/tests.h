/*
 * Generic queue manipulation functions
 */
#ifndef __TESTS_H__
#define __TESTS_H__

/*
 * Queue tests needed
 */

#include "../queue.h"
#include "../multilevel_queue.h"

struct node {
    struct node* previous;
    struct node* next;
    void* item;
};

struct queue {
    node_t* head;
    node_t* tail;
    int size;
};

typedef struct item {
	int val;
} item_t;


#endif /*__QUEUE_H__*/


