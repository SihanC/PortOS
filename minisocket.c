/*
 *  Implementation of minisockets.
 */

#include "alarm.h"
#include "minisocket.h"
#include "minimsg.h"
#include "miniheader.h"
#include "network.h"
#include "queue.h"
#include "interrupts.h"
#include "synch.h"
#include "string.h"

#define MAX_PORT_NUMBER 65535
#define CLIENT_PORT_START 32768 
#define INITIAL_TIMEOUT 0.1
#define RELIABLE_HDR_LEN sizeof(mini_header_reliable_t)
#define MAX_TCP_MSG MAX_NETWORK_PKT_SIZE - RELIABLE_HDR_LEN
#define START_BUF_SIZE 100

struct minisocket {
    enum {LISTEN, SEND_SYN, SEND_SYNACK, ESTABLISHED, DATA_SENT, ACK_RECEIVED, SEND_FIN, SEND_FINACK} status;
    char message_type;
    int local_port_number;
    int remote_port_number;
    network_address_t local_address;
    network_address_t remote_address;   
    int seq_number;
    int ack_number;
    int incoming_ack;
    queue_t *incoming_data;
    semaphore_t *ack_ready;          // Used to notify senxd_message ack received, stop retrying. 
    semaphore_t *datagrams_ready;    // Used in minisocket_receive
};

struct minimessage {
    int size;
    char buffer[MAX_TCP_MSG];
};

minisocket_t *sockets[MAX_PORT_NUMBER];
network_address_t host_address;
int next_port_number = CLIENT_PORT_START;
int total_client_port;

/* Helper functions */
// Pack a TCP header
mini_header_reliable_t* pack_header(minisocket_t *socket, char message_type, minisocket_error* error) {
    if (error == NULL) {
        return NULL;
    }

    mini_header_reliable_t* header = (mini_header_reliable_t*) malloc(sizeof(mini_header_reliable_t));
    if (header == NULL) {
        *error = SOCKET_OUTOFMEMORY;
        return NULL;
    }
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    header->protocol = PROTOCOL_MINISTREAM;
    pack_address(header->source_address, host_address);
    pack_unsigned_short(header->source_port, socket->local_port_number);
    pack_address(header->destination_address, socket->remote_address);
    pack_unsigned_short(header->destination_port, socket->remote_port_number);
    header->message_type = message_type;
    pack_unsigned_int(header->seq_number, socket->seq_number);
    pack_unsigned_int(header->ack_number, socket->ack_number);
    *error = SOCKET_NOERROR;
    set_interrupt_level(old_level);
    return header;
}

// Send message with timeout. Return size of packet successfully delivered or -1 when failure 
int send_message(minisocket_t *socket, mini_header_reliable_t *header, int msg_len, char *msg, minisocket_error* error) {
    double timeout = INITIAL_TIMEOUT;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    while (timeout <= 6.4) {
        int sent_size = network_send_pkt(socket->remote_address, RELIABLE_HDR_LEN, (char*) header, msg_len, msg);
        alarm_id a = alarm_register(timeout * SECOND / MILLISECOND, (void (*)(void *))semaphore_V, socket-> ack_ready);
        // putting thread on wait queue for port
        semaphore_P(socket->ack_ready);
        
        if (socket->message_type == MSG_FIN && socket->status == SEND_SYN) {
            *error = SOCKET_BUSY;
            return -1;
        }

        // Received ACK
        if (socket->incoming_ack == socket->seq_number) {
            alarm_deregister(a);
            return sent_size - RELIABLE_HDR_LEN;
        }
        timeout *= 2;
    }
    *error = SOCKET_NOSERVER;
    set_interrupt_level(old_level);
    return -1;
}

// Destroy socket
void minisocket_destroy(minisocket_t *socket) {
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    if (socket->local_port_number >= CLIENT_PORT_START) {
        total_client_port--;
    }
    semaphore_destroy(socket->ack_ready);
    semaphore_destroy(socket->datagrams_ready);
    while (queue_length(socket->incoming_data) > 0) {
         minimessage_t *minimsg = (minimessage_t *) malloc(sizeof(minimessage_t));
        assert(minimsg != NULL);
        int ret = queue_dequeue(socket->incoming_data, (void **) &minimsg);
        assert(ret != -1);
        //free(minimsg->buffer);
        free(minimsg);
    }
    queue_free(socket->incoming_data);
    free(socket);
    set_interrupt_level(old_level);
}

// Return -1 when error occurred, sent packet size otherwise.
int send_ACK(minisocket_t *socket, minisocket_error *error) {
    if (socket == NULL || error == NULL) 
        return -1;
    mini_header_reliable_t *header;
    header = pack_header(socket, MSG_ACK, error);
    if (header == NULL) 
        return -1;
    return network_send_pkt(socket -> remote_address, RELIABLE_HDR_LEN, (char*) header, 0, "");
}


/* minisocket functions */
void minisocket_initialize() {
    network_get_my_address(host_address);
}

minisocket_t* minisocket_server_create(int port, minisocket_error *error) {
    // Check for errors in inputs
    if (error == NULL) {
        return NULL;
    } else if (port < 0 || port >= CLIENT_PORT_START) {
        *error = SOCKET_INVALIDPARAMS;
        return NULL;        
    } else if (sockets[port] != NULL) {
        *error = SOCKET_PORTINUSE;
        return NULL;
    }

    // Set up a new server
    sockets[port] = (minisocket_t*) malloc(sizeof(minisocket_t));
    if (sockets[port] == NULL) {
        *error = SOCKET_OUTOFMEMORY;
        return NULL;
    }
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    sockets[port]->local_port_number = port;
    network_address_copy(host_address, sockets[port]->local_address);

    sockets[port]->incoming_data = queue_new();
    sockets[port]->ack_ready = semaphore_create();
    sockets[port]->datagrams_ready = semaphore_create();
    
    // fail to create queue or semaphore
    if (sockets[port]->incoming_data == NULL || sockets[port]->ack_ready == NULL \
        || sockets[port]->datagrams_ready == NULL) {
        minisocket_close(sockets[port]) ;
    }

    while (1) {
        sockets[port]->seq_number = 0;
        sockets[port]->ack_number = 0;
        semaphore_initialize(sockets[port]->ack_ready, 0);
        semaphore_initialize(sockets[port]->datagrams_ready, 0);
        
        // Start to listen (wait for the connection) 
        sockets[port]->status = LISTEN;
        semaphore_P(sockets[port]->datagrams_ready);

        // Received MSG_SYN and send MSG_SYNACK
        sockets[port]->seq_number++; 
        mini_header_reliable_t *header;
        header = pack_header(sockets[port], MSG_SYNACK, error);
        if (header == NULL) 
            return NULL; 
        sockets[port]->status = SEND_SYNACK;
        int ret = send_message(sockets[port], header, 0, "", error);
        if (ret != -1) {
            sockets[port]->status = ESTABLISHED;
            break;
        }
    }
    *error = SOCKET_NOERROR;
    set_interrupt_level(old_level);
    return sockets[port];
}

minisocket_t* minisocket_client_create(const network_address_t addr, int port, minisocket_error *error) {
    // Check for errors in inputs
    if (error == NULL) {
        return NULL;
    } else if (addr == NULL || port < 0 || port >= CLIENT_PORT_START) {
        *error = SOCKET_INVALIDPARAMS;
        return NULL;
    } else if (total_client_port >= MAX_PORT_NUMBER - CLIENT_PORT_START) {
        *error = SOCKET_NOMOREPORTS;
        return NULL;
    }

    // Search for next available ports
    while (sockets[next_port_number] != NULL) {
        next_port_number = (next_port_number + 1) % CLIENT_PORT_START + CLIENT_PORT_START;
    }

    // Set up new client
    sockets[next_port_number] = (minisocket_t*) malloc(sizeof (minisocket_t));
    if (sockets[next_port_number] == NULL) {
        *error = SOCKET_OUTOFMEMORY;
        return NULL;
    }
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    minisocket_t *socket = sockets[next_port_number];

    socket->seq_number = 1; 
    socket->ack_number = 0; 
    socket->local_port_number = next_port_number;
    network_address_copy(host_address, socket->local_address);
    socket->remote_port_number = port;
    network_address_copy(addr, socket->remote_address);
    socket->incoming_data = queue_new();
    socket->ack_ready = semaphore_create();
    socket->datagrams_ready = semaphore_create();
    // Fail to create queue or semaphore
    if (socket->incoming_data == NULL || socket->ack_ready == NULL \
        || socket->datagrams_ready == NULL) {
        minisocket_destroy(socket);
        *error = SOCKET_OUTOFMEMORY;
        return NULL;
    }
    semaphore_initialize(socket->ack_ready, 0);
    semaphore_initialize(socket->datagrams_ready, 0);
    total_client_port++;

    // send SYN
    mini_header_reliable_t *header;
    header = pack_header(socket, MSG_SYN, error);
    if (header == NULL) 
        return NULL;
    socket->status = SEND_SYN;
    int ret = send_message(socket, header, 0, "", error);
    if (ret == -1) {
        return NULL;
    }

    // Received SYNACK and send MSG_ACK
    // Only send MSG_ACK once in here, if packet loss, other MSG_ACK will be sent in minisocket_append switch statement. 
    send_ACK(socket, error);
    socket->status = ESTABLISHED;
    *error = SOCKET_NOERROR;
    set_interrupt_level(old_level);
    return socket;
}

int minisocket_send(minisocket_t *socket, const char *msg, int len, minisocket_error *error) {
    // Check for errors in inputs
    if (error == NULL) {
        return -1;
    } else if (socket == NULL) {
        *error = SOCKET_SENDERROR;
        return -1;
    } else if (msg == NULL || len < 0) {
        *error = SOCKET_INVALIDPARAMS;
        return -1;
    } else if (socket -> status == SEND_FIN) {
        *error = SOCKET_SENDERROR;
        return -1;
    }

    int max_tcp_msg = MAX_TCP_MSG, total_sent = 0, ret;
    char tmp_msg[len];
    memcpy(tmp_msg, msg, len);
    mini_header_reliable_t *header;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    
    // Fragment packet and send
    while (len > 0) {
        socket -> seq_number += 1;
        socket -> status = DATA_SENT;
        header = pack_header(socket, MSG_ACK, error);
        if (header == NULL) 
            return -1; 
        if (len > max_tcp_msg) {
            // Message can't fit in one packet
            ret = send_message(socket, header, max_tcp_msg, tmp_msg + total_sent, error);
            if (ret == -1)
                return total_sent;
            total_sent += ret; 
            len = len - ret;
        } else {
            ret = send_message(socket, header, len, tmp_msg + total_sent, error);
            if (ret == -1)
                return total_sent;
            total_sent += ret;
            break;
        }   
    }
    set_interrupt_level(old_level);
    return total_sent;
}

int minisocket_receive(minisocket_t *socket, char *msg, int max_len, minisocket_error *error) {
    // Check parameters and initialize minimsg
    if (error == NULL) {
        return -1;
    } else if (socket == NULL || msg == NULL || max_len < 0) {
        *error = SOCKET_INVALIDPARAMS;
        return -1;
    } else if (socket -> status == SEND_FINACK) {
        *error = SOCKET_RECEIVEERROR;
        return -1;
    }

    minimessage_t *minimsg = (minimessage_t *) malloc(sizeof(minimessage_t));
    if (minimsg == NULL) {
        *error = SOCKET_OUTOFMEMORY;
        return -1;
     }

    // Wait for new message to arrive
    int size, ret;
    int true_data = 0;
    interrupt_level_t old_level;
    while (!true_data) {
        semaphore_P(socket -> datagrams_ready);
        true_data = 1;
        old_level = set_interrupt_level(DISABLED);
        ret = queue_dequeue(socket -> incoming_data, (void **) &minimsg);
        assert(ret != -1);
        if (socket -> message_type == MSG_FIN) {
            mini_header_reliable_t *header;
            socket -> status = SEND_FINACK;
            header = pack_header(socket, MSG_FINACK, error);
            if (header == NULL) 
                return -1;
            network_send_pkt(socket -> remote_address, RELIABLE_HDR_LEN, (char *) header, 0, "");
            minisocket_destroy(socket);
            return 0;
        } else if (socket -> message_type == MSG_SYN) {
            mini_header_reliable_t *header;
            header = pack_header(socket, MSG_FIN, error);
            if (header == NULL) 
                return -1;
            network_send_pkt(socket -> remote_address, RELIABLE_HDR_LEN, (char *) header, 0, "");
            true_data = 0;
        }
    }

    if (max_len < minimsg -> size) {  // partial receive
        memcpy(msg, minimsg -> buffer, max_len);
        size = max_len;
        memcpy(minimsg -> buffer, (minimsg -> buffer) + size, minimsg -> size - size);
        minimsg -> size -= size;  
        ret = queue_prepend(socket -> incoming_data, (void *) minimsg);
        semaphore_V(socket -> datagrams_ready);
        assert(ret != -1);
    } else {
        size = minimsg -> size;
        memcpy(msg, minimsg -> buffer, size);
        free(minimsg);
    }

    // Acknowledge message
    set_interrupt_level(old_level);
    return size;
}

 void minisocket_close(minisocket_t *socket) {
    if (socket == NULL) 
        return;
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    socket -> seq_number++;
    set_interrupt_level(old_level);
    minisocket_error *error = (minisocket_error *) malloc(sizeof(minisocket_error));
    mini_header_reliable_t *header = pack_header(socket, MSG_FIN, error);
    if (header == NULL) 
        return; 
    socket -> status = SEND_FIN;
    send_message(socket, header, 0, "", error);
    free (error);
}

void minisocket_append(network_interrupt_arg_t* packet) {
    mini_header_reliable_t *header = (mini_header_reliable_t *) packet -> buffer;
    minimessage_t *minimsg = (minimessage_t *) malloc(sizeof(minimessage_t));
    assert(minimsg != NULL);
    interrupt_level_t old_level = set_interrupt_level(DISABLED);
    minimsg -> size = packet -> size - RELIABLE_HDR_LEN;
    memcpy(minimsg -> buffer, (packet -> buffer) + RELIABLE_HDR_LEN , minimsg -> size);
    int port_number = unpack_unsigned_short(header -> destination_port);
    if (port_number < 0 || port_number > MAX_PORT_NUMBER) {
        free(minimsg);
        return;
    }
    minisocket_t *socket = sockets[port_number];
    assert(socket != NULL);
    minisocket_error error;    // place holder
    socket -> incoming_ack = unpack_unsigned_int(header -> ack_number);
    int incoming_seq = unpack_unsigned_int(header -> seq_number);
    switch (header -> message_type) {
        case MSG_SYN:
            socket -> message_type = MSG_SYN;
            socket -> remote_port_number = unpack_unsigned_short(header -> source_port);
            socket -> ack_number = unpack_unsigned_int(header -> seq_number);
            unpack_address(header -> source_address, socket -> remote_address);
            semaphore_V(socket -> datagrams_ready);
            break;
        case MSG_SYNACK: 
            if (socket -> status == ESTABLISHED) {
                // Duplicate SYNACK, MSG_ACK might be lost, send again.
                send_ACK(socket, &error);
            } else {
                socket -> message_type = MSG_SYNACK;
                socket -> ack_number = 1;
                semaphore_V(socket -> ack_ready);
            }
            break;
        case MSG_ACK:
            if (socket -> status == LISTEN) 
                break;
            socket -> message_type = MSG_ACK;
            // receive ACK / Data as ACK
            if (socket -> incoming_ack == socket -> seq_number && 
                socket -> status == DATA_SENT) {
                semaphore_V(socket -> ack_ready);     
            }

            if (minimsg -> size != 0) {   
                if (incoming_seq > socket -> ack_number) {
                    int ret = queue_append(socket -> incoming_data, (void *) minimsg);
                    assert(ret == 0);
                    socket -> ack_number = incoming_seq;
                    semaphore_V(socket -> datagrams_ready);           
                }
                // send ACK for both dulicate and not-duplicate data
                send_ACK(socket, &error);
            }
            break;
        case MSG_FIN:
            socket -> message_type = MSG_FIN;
            if (socket -> status == SEND_SYN) {
                semaphore_V(socket -> ack_ready);
            } else {
                semaphore_V(socket -> datagrams_ready);
            }
            break;
        case MSG_FINACK:
            socket -> message_type = MSG_FINACK;
            if (socket -> status == SEND_FIN) {
                semaphore_V(socket -> ack_ready);
                send_ACK(socket, &error);
            }   
            break;  
    }
    set_interrupt_level(old_level);
}

