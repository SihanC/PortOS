/*
 * Multilevel queue manipulation functions
 */

#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "multilevel_queue.h"

struct multilevel_queue {
    int length;
    int total_level;
    queue_t **mul_queue;
};

multilevel_queue_t* multilevel_queue_new(int number_of_levels) {
    if (number_of_levels < 0) 
        return NULL;

    multilevel_queue_t *m_q;
    if ((m_q = (multilevel_queue_t *) malloc(sizeof(multilevel_queue_t))) == NULL) { 
        fprintf(stderr, "Failed to allocate memory for multilevel_queue\n");
        return NULL; 
    }
    m_q -> length = 0;
    m_q -> total_level = number_of_levels;
    m_q -> mul_queue = (queue_t **) malloc(sizeof(queue_t *) * number_of_levels); 
    if (m_q -> mul_queue == NULL) {
        fprintf(stderr, "Failed to allocate memory for each queue\n");
        free(m_q);
        return NULL;
    }

    int i, j;
    for (i = 0; i < number_of_levels; i++) {
        m_q -> mul_queue[i] = queue_new();
        if (m_q -> mul_queue[i] == NULL) {
            fprintf(stderr, "Failed to create new queue\n");
            for (j = 0; j < i; j++) { 
                free(m_q -> mul_queue[j]); 
            }
            free(m_q -> mul_queue);
            free(m_q);
            return NULL;
        }
    }
    return m_q;
}

int multilevel_queue_enqueue(multilevel_queue_t *queue, int level, void* item) {
    if (queue == NULL || level < 0 || level >= queue -> total_level 
        || item == NULL) 
        return -1;

    if (queue_append(queue -> mul_queue[level], item) == -1) {
        fprintf(stderr, "Failed to append to the queue");
        return -1;
    }
    queue -> length++;
    return 0;
}

int multilevel_queue_dequeue(multilevel_queue_t *queue, int level, void** item) {
    if (queue == NULL || level < 0 || 
        level >= queue -> total_level || item == NULL) 
        return -1;

    int i = level;
    while (i < queue -> total_level) {
        if (queue_dequeue(queue -> mul_queue[i], item) == 0) {
            queue -> length--;
            return i;
        }
        i++;
    }

    i = 0;
    while (i < level) {
        if (queue_dequeue(queue->mul_queue[i], item) == 0) {
            queue -> length--;
            return i;
        }
        i++;
    }
    return -1;
}

int multilevel_queue_free(multilevel_queue_t *queue) {
    if (queue == NULL) 
        return -1;
    
    int i;
    for (i = 0; i < queue -> total_level; i++) {
        queue_free(queue -> mul_queue[i]);
    }
    free(queue -> mul_queue);
    free(queue);
    return 0;
}

queue_t* multilevel_queue_getq(multilevel_queue_t * queue, int level) {
    if (level < 0 || level >= queue -> total_level) 
        return NULL;
    
    return queue -> mul_queue[level];
}

int multilevel_queue_length(multilevel_queue_t* queue) {
    if (queue == NULL) 
        return 0;

    return queue -> length;
}
