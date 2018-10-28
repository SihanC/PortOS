#include <stdio.h>
#include <math.h>

#include "interrupts.h"
#include "alarm.h"
#include "queue.h"
#include "minithread.h"

/* Definition of alarm */
struct alarm {
    int end_time;
    alarm_handler_t func;
    void* arg;
};

/*
 * Compare the end time of two alarms.
 * Return 0 if equal, 1 if a1 > a2, -1 if a2 > a1
 */
int compare_alarms(void* a1, void* a2) {
    int t1 = (*(alarm_t *) a1).end_time;
    int t2 = (*(alarm_t *) a2).end_time;
    if (t1 == t2) {
        return 0;
    } else if (t1 > t2) {
        return 1;
    } else {
        return -1;
    }
}

void do_alarms() {
    alarm_handler_t f;
    alarm_t *curr_alarm = queue_peek(alarm_list);
    while (curr_alarm != NULL && curr_alarm -> end_time <= ticks) {
        f = curr_alarm -> func;
        f(curr_alarm -> arg);
        int ret = queue_dequeue(alarm_list, (void **) &curr_alarm);
        assert(ret != -1);
        curr_alarm = queue_peek(alarm_list);
    }
}

alarm_id alarm_register(int delay, alarm_handler_t alarm, void *arg) {
    alarm_t* a = (alarm_t*) malloc(sizeof(alarm_t));
    if (a == NULL) {
        fprintf(stderr, "Failed to register alarm\n");
        return NULL;
    }
    int r = (delay * MILLISECOND) / PERIOD;
    // Delay = 0, handle it on the next clock tick
    if ((delay == 0) || (delay * MILLISECOND) % PERIOD != 0) {
        // End-time in between two ticks will round up to next tick
        r++;
    }
    a -> end_time = ticks + r;
    a -> func = alarm;
    a -> arg = arg;
    queue_sorted_insert(alarm_list, (void*) a, compare_alarms);
    return a;
}

int alarm_deregister(alarm_id alarm) {
    int end_time = ((alarm_t*) alarm) -> end_time;
    queue_delete(alarm_list, (void*) alarm);
    free(alarm);
    return end_time <= ticks;
}