/* alarm_test.c
 *
 * Spawn three threads, all of them never yield. 
 */

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int thread3(int* arg) {
  printf("Thread 3 does not going to sleep.\n");
  return 0;
}

int thread2(int* arg) {
  minithread_fork(thread3, NULL);
  printf("Thread 2 going to sleep for 100s.\n");
  minithread_sleep_with_timeout(100);
  printf("Thread 2 Wake up after timeout.\n");
  return 0;
}

int thread1(int* arg) {
  minithread_fork(thread2, NULL);
  printf("Thread 1 going to sleep for 5000s.\n");
  minithread_sleep_with_timeout(5000);
  printf("Thread 1 Wake up after timeout.\n");
  return 0;
}

int
main(int argc, char * argv[]) {
  minithread_system_initialize(thread1, NULL);
  return 0;
}
