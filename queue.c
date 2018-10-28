/*
 * Generic queue implementation.
 */

#include <stdlib.h>
#include <stdio.h>

#include "queue.h"

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

void* queue_peek(queue_t *q) {
    if (q == NULL || q -> head == NULL)
        return NULL;
    return q -> head -> item;
}

queue_t* queue_new() {
    queue_t *tQueue = (queue_t *) malloc(sizeof(queue_t));
    if (tQueue == NULL) 
        return NULL;
    tQueue -> head = NULL;
    tQueue -> tail = NULL;
    tQueue -> size = 0; 
    return tQueue;
}

int queue_prepend(queue_t *queue, void* item) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (node == NULL || queue == NULL || item == NULL) 
        return -1;

    node -> previous = NULL;
    node -> next = queue -> head;
    node -> item = item;
    if (queue -> head != NULL)
        queue -> head -> previous = node;
    else
        queue -> tail = node;    
    (queue -> size)++;
    queue -> head = node;
    return 0;
}

int queue_append(queue_t *queue, void* item) {
    if (queue == NULL || item == NULL)
        return -1;

    node_t *node = (node_t *) malloc(sizeof(node_t));
    if (node == NULL) 
        return -1;
    
    node -> next = NULL;
    node -> previous = queue -> tail;
    node -> item = item;
    if (queue -> tail != NULL)
        queue -> tail -> next = node;
    else
        queue -> head = node;
    (queue -> size)++;
    queue -> tail = node;
    return 0;
}

int queue_dequeue(queue_t *queue, void** item) {
    if (item == NULL) 
        return -1;

    if (queue == NULL || queue -> size == 0) {
        *item = NULL;
        return -1;
    }

    *item = queue -> head -> item;
    if (queue -> size == 1) {
        queue -> head = NULL;
        queue -> tail = NULL;
    } else if (queue -> size > 1) {
        queue -> head = queue -> head -> next;
        queue -> head -> previous = NULL;
    }   
    (queue -> size)--;
    return 0;
}

int queue_iterate(queue_t *queue, func_t f, void* arg) {
    if (queue == NULL || f == NULL) 
        return -1;

    node_t *curr = queue -> head;
    while (curr != NULL) {
        node_t * tmp = curr -> next;
        f(curr -> item, arg);
        curr = tmp;
    }
    return 0;
}

int queue_free(queue_t *queue) {
    if (queue == NULL || queue -> size > 0) 
        return -1;

    free(queue);
    return 0;
}

int queue_length(const queue_t *queue) {
    if (queue == NULL) 
        return -1;

    else return queue -> size;
}

int queue_delete(queue_t *queue, void* item) {
    if (queue == NULL || item == NULL) 
        return -1;

    node_t *curr = queue -> head;
    while (curr != NULL) {
        if (curr -> item == item) {
            if (queue -> size == 1) {
                queue -> head = NULL;
                queue -> tail = NULL;
            } else if (curr == queue -> head) {
                curr -> next -> previous = NULL;
                queue -> head = curr -> next;
            } else if (curr == queue -> tail) {
                curr -> previous -> next = NULL;
                queue -> tail = curr -> previous;
            } else {
                curr -> previous -> next = curr -> next;
                curr -> next -> previous = curr -> previous;
            }
            free(curr);
            (queue -> size)--;
            return 0;
        }
        curr = curr -> next;
    }
    return -1;
}

int queue_sorted_insert(queue_t* queue, void* item, comp_t f) {
    if (queue == NULL || item == NULL || f == NULL) 
        return -1;

    node_t *curr = queue -> head;
    node_t *new = (node_t *) malloc(sizeof(node_t));
    new -> item = item;
    new -> previous = NULL;
    new -> next = NULL;
    if (curr == NULL) 
        queue -> head = new;
    (queue -> size)++;

    while (curr != NULL) {
        if (f(curr -> item, item) >= 0) {
            new -> previous = curr -> previous;
            new -> next = curr;
            if (curr -> previous != NULL)
                curr -> previous -> next = new;
            curr -> previous = new;
            if (queue -> head == curr) {
                queue -> head = new;
            }
            return 0;
        } else {
            curr = curr -> next;
        }
    }

    // new item is later than all other items 
    if (queue -> tail != NULL) {
        queue -> tail -> next = new;
        new -> previous = queue -> tail;
    }
    queue -> tail = new;
    return 0;
}

