/*
 *  Implementation of minimsgs and miniports.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "minimsg.h"
#include "miniheader.h"
#include "queue.h"
#include "interrupts.h"
#include "synch.h"

#define MAX_PORT_NUMBER 65536
#define BOUND_PORT_START 32768

typedef enum {BOUND_PORT, UNBOUND_PORT} port_type_t;
struct miniport {
    int port_number;
    port_type_t type;
    union {
        struct {
            network_address_t remote_address;
            int remote_unbound_port;
        } bound;
        struct {
            queue_t *incoming_data;
            semaphore_t *datagrams_ready;
        } unbound;
    };
};

struct minimessage {
    int size;
    char buffer[MAX_NETWORK_PKT_SIZE];
};

int next_bound_port_number;
int total_bound_port;
miniport_t *ports[MAX_PORT_NUMBER];
network_address_t host_address;

void minimsg_initialize() {
    next_bound_port_number = BOUND_PORT_START;
    total_bound_port = 0;
    network_get_my_address(host_address);
}

miniport_t* miniport_create_unbound(int port_number) {
    if (port_number < 0 || port_number >= BOUND_PORT_START)
        return NULL;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (ports[port_number] == NULL) {
        ports[port_number] = (miniport_t*) malloc(sizeof(miniport_t));
        assert(ports[port_number] != NULL);
        ports[port_number] -> port_number = port_number;
        ports[port_number] -> type = UNBOUND_PORT;
        ports[port_number] -> unbound.incoming_data = queue_new();
        ports[port_number] -> unbound.datagrams_ready = semaphore_create();

        // Fail to create queue or semaphore
        if (ports[port_number] -> unbound.incoming_data == NULL \
            || ports[port_number] -> unbound.datagrams_ready == NULL) {
            miniport_destroy(ports[port_number]);
        }
        semaphore_initialize(ports[port_number] -> unbound.datagrams_ready, 0);
    }
    set_interrupt_level(old_level);
    return ports[port_number];
}

miniport_t* miniport_create_bound(const network_address_t addr, int remote_unbound_port_number) {
    if (addr == NULL || remote_unbound_port_number < 0 || remote_unbound_port_number >= BOUND_PORT_START) 
        return NULL;

    // Search for next available port
    if (total_bound_port >= MAX_PORT_NUMBER - BOUND_PORT_START)
        return NULL;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    while (ports[next_bound_port_number] != NULL) {
        next_bound_port_number = (next_bound_port_number + 1) % BOUND_PORT_START + BOUND_PORT_START;
    }
    ports[next_bound_port_number] = (miniport_t*) malloc(sizeof(miniport_t));
    assert(ports[ next_bound_port_number ] != NULL );
    ports[next_bound_port_number] -> port_number = next_bound_port_number;
    ports[next_bound_port_number] -> type = BOUND_PORT;
    network_address_copy(addr, ports[next_bound_port_number] -> bound.remote_address);
    ports[next_bound_port_number] -> bound.remote_unbound_port = remote_unbound_port_number;
    total_bound_port++;
    miniport_t *tmp = ports[next_bound_port_number];
    set_interrupt_level(old_level);
    return tmp;
}

void miniport_destroy(miniport_t* miniport) {
    if (miniport == NULL)
        return;
    int p_num = miniport -> port_number;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (miniport -> type == BOUND_PORT) {
        total_bound_port--;
    } else {
        queue_free(ports[p_num] -> unbound.incoming_data);
        semaphore_destroy(ports[p_num] -> unbound.datagrams_ready);
    }
    free(ports[p_num]);
    ports[p_num] = NULL;
    set_interrupt_level(old_level);
}

/*
 * Return -1 when fail to send a message, otherwise return the length of the message
 */
int minimsg_send(miniport_t* local_unbound_port, const miniport_t* local_bound_port, const char* msg, int len) {
    if (local_unbound_port == NULL || local_bound_port == NULL || msg == NULL || len > MINIMSG_MAX_MSG_SIZE)
        return -1;
    mini_header_t *header = ( mini_header_t *) malloc(sizeof(mini_header_t));
    header -> protocol = PROTOCOL_MINIDATAGRAM;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    pack_address(header -> source_address, host_address);
    pack_address(header -> destination_address, local_bound_port -> bound.remote_address);
    pack_unsigned_short(header -> source_port, local_unbound_port -> port_number);
    pack_unsigned_short(header -> destination_port, local_bound_port -> bound.remote_unbound_port);
    int size = network_send_pkt(local_bound_port -> bound.remote_address, sizeof(mini_header_t), (char *) header, len, msg) - sizeof(mini_header_t);
    set_interrupt_level(old_level);
    return size;
}

// Return -1 when some error happen, 0 otherwise
int minimsg_receive(miniport_t* local_unbound_port, miniport_t** new_local_bound_port, char* msg, int *len) {
    if (local_unbound_port == NULL || new_local_bound_port == NULL || msg == NULL || len == NULL) 
        return -1;

    // Block until a message being received
    semaphore_P(local_unbound_port -> unbound.datagrams_ready);
    
    // Retrieve message 
    minimessage_t *minimsg;

    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    int ret = queue_dequeue(local_unbound_port -> unbound.incoming_data, (void **) &minimsg);
    assert(ret == 0);
    
    // Unpack header
    mini_header_t *header = (mini_header_t *)minimsg -> buffer;
    network_address_t dest_addr; 
    unpack_address(header -> source_address, dest_addr);
    int remote_port = unpack_unsigned_short(header -> source_port);
    assert(remote_port >= 0 && remote_port <= 32767);
    *new_local_bound_port = miniport_create_bound(dest_addr, remote_port);
    assert(*new_local_bound_port != NULL);

    // Strip header
    memcpy(msg, minimsg -> buffer + sizeof(mini_header_t), *len);
    set_interrupt_level(old_level);
    return 0;
}

void minimsg_append(network_interrupt_arg_t* packet) {
    mini_header_t *header = (mini_header_t *) packet -> buffer;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    int port_number = unpack_unsigned_short(header -> destination_port);
    minimessage_t *minimsg = (minimessage_t *) malloc(sizeof(minimessage_t));
    minimsg -> size = packet -> size - sizeof(mini_header_t);
    memcpy(minimsg -> buffer, (packet -> buffer), sizeof(packet -> buffer));
    int ret = queue_append(ports[port_number] -> unbound.incoming_data, (void *) minimsg);
    assert(ret == 0);
    semaphore_V(ports[port_number] -> unbound.datagrams_ready);
    set_interrupt_level(old_level);
} 
