/* alarm_test_simple.c
 *
 * Spawn one threads, all of them never yield. 
 */

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int thread1(int* arg) {
  printf("Thread 1 going to sleep for 5ms.\n");
  minithread_sleep_with_timeout(5000);
  printf("Thread 1 Wake up after timeout.\n");
  return 0;
}

int
main(int argc, char * argv[]) {
  minithread_system_initialize(thread1, NULL);
  return 0;
}
