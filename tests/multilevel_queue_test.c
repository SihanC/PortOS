#include <stdlib.h>
#include <stdio.h>

#include "tests.h"

/* Definition of multilevel queue */
typedef struct multilevel_queue {
    int total_level;
    queue_t **mul_queue;
} multilevel_queue_t;

/* Helper function to print out queue */
void printq(queue_t* q, int i) {
    if (q != NULL && q -> size != 0) {
        printf("Queue %d length is: %d\n", i, queue_length(q));
        node_t* curr = q -> head;
        item_t *x = (item_t *) (curr -> item);
        while (curr != NULL) {
            item_t *x = (item_t *) (curr -> item);
            if (curr -> next == NULL) 
                printf("%d", x->val);    
            else
                printf("%d, ", x->val);
            curr = curr -> next;
        }
        printf("\n");
    } else {
        printf("Queue %d is empty.\n", i);
    }
}

/* Helper function to print out multi-level queue */
void multilevel_queue_printq(multilevel_queue_t *q, int level) {
    int i;
    for (i = 0; i < level; i++) {
        printq(multilevel_queue_getq(q, i), i);
    } 
}

int main() {
    item_t item1 = {1};
    item_t item2 = {2};
    item_t item3 = {3};
    item_t item4 = {4};
    item_t item5 = {5};
    item_t item6 = {6};
    item_t item7 = {7};
    item_t item8 = {8};
    item_t item9 = {9};
    item_t item10 = {10};
    item_t item11 = {11};
    item_t item12 = {12};

    // Test multilevel_queue_new 
    printf("Testing multilevel_queue_new\n");
    multilevel_queue_t *m_q = multilevel_queue_new(4);
    multilevel_queue_printq(m_q, 4);

    // Test multilevel_queue_enqueue
    // Q0: 1 2 3
    // Q1
    // Q2: 4 5 6
    // Q3: 7 8 9 10
    printf("\n\nTesting multilevel_queue_enqueue\n");
    multilevel_queue_enqueue(m_q, 0, &item1);
    multilevel_queue_enqueue(m_q, 0, &item2);
    multilevel_queue_enqueue(m_q, 0, &item3);
    multilevel_queue_enqueue(m_q, 2, &item4);
    multilevel_queue_enqueue(m_q, 2, &item5);
    multilevel_queue_enqueue(m_q, 2, &item6);
    multilevel_queue_enqueue(m_q, 3, &item7);
    multilevel_queue_enqueue(m_q, 3, &item8);
    multilevel_queue_enqueue(m_q, 3, &item9);
    multilevel_queue_enqueue(m_q, 3, &item10);
    multilevel_queue_printq(m_q, 4);
 
    // Test multilevel_queue_length
    printf("\n\nTesting multilevel_queue_length\n");
    printf("Length is: %d\n", multilevel_queue_length(m_q));

    // Test multilevel_queue_dequeue
    printf("\n\nTesting multilevel_queue_dequeue\n");
    int level;
    item_t *x;
    level = multilevel_queue_dequeue(m_q, 1, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x->val, level);
    level = multilevel_queue_dequeue(m_q, 1, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x->val, level);
    level = multilevel_queue_dequeue(m_q, 1, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x->val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x->val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x->val, level);

    printf("\n\n");
    multilevel_queue_enqueue(m_q, 0, &item4);
    multilevel_queue_printq(m_q, 4);
    printf("\n");
    multilevel_queue_enqueue(m_q, 1, &item7);
    multilevel_queue_printq(m_q, 4);
    
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("dequeue item is: %d, level is : %d\n\n", x -> val, level);
    level = multilevel_queue_dequeue( m_q, 0, (void **)&x );
    multilevel_queue_printq( m_q, 4 );
    printf( "dequeue item is: %d, level is : %d\n\n", x -> val, level );
    level = multilevel_queue_dequeue(m_q, 0, (void **)&x);
    multilevel_queue_printq(m_q, 4);
    printf("level is : %d\n\n", level);

    multilevel_queue_enqueue(m_q, 1, &item7);
    multilevel_queue_printq(m_q, 4);
    return 0;
} 