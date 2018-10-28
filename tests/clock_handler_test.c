/* clock_handler_test.c
 *
 * Spawn three threads, all of them never yield. If clock handler works correctly
 * thread 1 will be interrupted and thread2 and 3 will run and print its string. 
 */

#include "minithread.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int thread3(int* arg) {
  printf("Thread 3.\n");
  return 0;
}

int thread2(int* arg) {
  minithread_fork(thread3, NULL);
  printf("Thread 2.\n");
  return 0;
}

int thread1(int* arg) {
  minithread_fork(thread2, NULL);
  printf("Thread 1.\n");
  int i = 0;
  while(1) {;
  	i++;
  }
  return 0;
}

int
main(int argc, char * argv[]) {
  minithread_system_initialize(thread1, NULL);
  return 0;
}
