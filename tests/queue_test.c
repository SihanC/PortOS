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

typedef struct item {
    int val;
} item_t;

void printq(queue_t* q) {
    if (q != NULL && q -> size != 0) {
        printf("Queue length is: %d\n", queue_length(q));
        node_t* curr = q -> head;
        while (curr != NULL) {
            if (curr == NULL)
                printf("curr is null\n");
            item_t *x = (item_t *) (curr -> item);
            if (curr -> next == NULL) 
                printf("%d", x->val);    
            else
                printf("%d, ", x->val);
            curr = curr -> next;
        }
        printf("\n");
    } else {
        printf("Queue is empty.\n");
    }
}

void f(void* item, void* arg) {
    item_t* x = (item_t*)item;
    item_t* y = (item_t*)arg;
    x -> val += y -> val;
}

int compare_int(void* a1, void* a2) {
    int t1 = ((item_t*) a1) -> val;
    int t2 = ((item_t*) a2) -> val;
    if (t1 == t2) {
        return 0;
    } else if (t1 > t2) {
        return 1;
    } else {
        return -1;
    }
}

int main() {
    // Test queue new()
    queue_t* q = queue_new();
    printq(q);

    // Test queue_prepend(), output should be 3 2 1
    item_t item1 = {1};
    item_t item2 = {2};
    item_t item3 = {3};
    item_t item4 = {4};
    item_t item5 = {5};
    item_t item6 = {6};
    void* p = &item1;
    queue_prepend(q, p);
    printq(q);
    p = &item2;
    queue_prepend(q, p);
    p = &item3;
    queue_prepend(q, p);
    printq(q);

    // test queue_append()
    // output should be 
    // 3 2 1 4
    p = &item4;
    queue_append(q, p);
    printq(q);

    // test queue_dequeue()
    // output should be 
    // 2 1 4
    // 1 4
    // 4
    void** d;
    d = malloc(sizeof(void *));
    queue_dequeue(q, d);
    printf("dequeue %d\n", ((item_t*)(*d)) -> val);
    printq(q);
    queue_dequeue(q, d);
    printq(q);
    printf("dequeue %d\n", ((item_t*)(*d)) -> val);
    queue_dequeue(q, d);
    printq(q);
    printf("dequeue %d\n", ((item_t*)(*d)) -> val);
    queue_dequeue(q, d);
    printq(q);
    printf("dequeue %d\n", ((item_t*)(*d)) -> val);
    int ret = queue_dequeue(q, d);
    printq(q);
    printf("dequeue %p\n", *d);
    printf("return value is %d, should be -1\n", ret);

    // rebuild the queue
    p = &item1;
    queue_prepend(q, p);
    p = &item2;
    queue_prepend(q, p);
    p = &item3;
    queue_prepend(q, p);
    p = &item4;
    queue_append(q, p);

    // test queue_delete
    // output should be 
    // 3 1 4
    p = &item2;
    queue_delete(q, p);
    printq(q);
    p = &item6;
    ret = queue_delete(q, p);
    printf("return code is: %d, should return -1\n", ret);
    printq(q);
    p = &item3;
    queue_delete(q, p);
    printq(q);

    // test queue_sorted_insert()
    // output should be
    // 1 2 3 4 5
    queue_t *qq = queue_new();
    p = &item2;
    queue_append(qq, p);
    p = &item3;
    queue_append(qq, p);
    p = &item4;
    queue_append(qq, p);
    p = &item5;
    queue_append(qq, p);
    printq(qq);

    p = &item1;
    printf("%d\n", queue_sorted_insert(qq, p, compare_int));
    printq(qq);
    return 0;   
}