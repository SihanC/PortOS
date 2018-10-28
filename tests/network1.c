/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

/* network test program 1

   local loopback test: sends and then receives one message on the same machine.

   USAGE: ./network1 <port>

   where <port> is the minimsg port number to use
   
   * The port number has to be valid and should not be in use.
       Try testing with port numbers larger than 4xxx.
*/

#include "minithread.h"
#include "minimsg.h"
#include "synch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#define BUFFER_SIZE 256


miniport_t *listen_port;
miniport_t *send_port;

int textlen = 14;
char text[] = "Hello, world!\n";

int thread(int* arg) {
    char buffer[BUFFER_SIZE];
    int length = BUFFER_SIZE;
    miniport_t *from;
    network_address_t my_address;

    network_get_my_address(my_address);
    listen_port = miniport_create_unbound(0);
    send_port = miniport_create_bound(my_address, 0);

    minimsg_send(listen_port, send_port, text, textlen);
    int ret= minimsg_receive(listen_port, &from, buffer, &length);
    printf("%d\n", ret);
    printf("%s", buffer);
    return 0;
}

int main(int argc, char** argv) {
    short fromport;
    fromport = atoi(argv[1]);
    network_udp_ports(fromport,fromport); 
    textlen = strlen(text) + 1;
    minithread_system_initialize(thread, NULL);
    return -1;
}


