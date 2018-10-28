/*
 * (C) 2017, Cornell University
 * All rights reserved.
 */

#ifndef __MINISOCKETS_H_
#define __MINISOCKETS_H_

/*
 *  Definitions for minisockets.
 *
 *      You should implement the functions defined in this file, using
 *      the names for types and functions defined here. Functions must take
 *      the exact arguments in the prototypes.
 *
 *      miniports and minisockets should coexist.
 */

#include <stdlib.h>
#include "network.h"
#include "minimsg.h"

typedef struct minisocket minisocket_t;
typedef enum minisocket_error minisocket_error;
typedef struct array_wrap array_wrap_t;


enum minisocket_error {
    SOCKET_NOERROR = 0,
    SOCKET_NOMOREPORTS,   /* ran out of free ports */
    SOCKET_PORTINUSE,     /* server tried to use a port that is already in use */
    SOCKET_NOSERVER,      /* client tried to connect to a port without a server */
    SOCKET_BUSY,          /* client tried to connect to a port that is in use */
    SOCKET_SENDERROR,
    SOCKET_RECEIVEERROR,
    SOCKET_INVALIDPARAMS, /* user supplied invalid parameters to the function */
    SOCKET_OUTOFMEMORY    /* function could not complete because of insufficient memory */
};

/* Initializes the minisocket layer. */
void minisocket_initialize();

/*
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t* minisocket_server_create(int port, minisocket_error *error);

/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine.
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t* minisocket_client_create(const network_address_t addr, int port, minisocket_error *error);

/*
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * all bytes of msg.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes,
 *               with any return value less than len signaling an error occurred
 *               Sets the proper error code in *error
 */
int minisocket_send(minisocket_t *socket, const char *msg, int len, minisocket_error *error);

/*
 * Receive data from the other end of the socket. Blocks until at least
 * 1 byte of data has been received or an error is encountered (e.g. socket closed)
 *
 * Returns at most max_len bytes of data
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: The number of bytes received ranging from [1, max_len] and the suitable error code
 *               in *error
 */
int minisocket_receive(minisocket_t* socket, char *msg, int max_len, minisocket_error *error);

/* 
 * Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t* socket);

/*
 * Append receving message to the queue of the specific socket.
 */
void minisocket_append(network_interrupt_arg_t* packet);
#endif /* __MINISOCKETS_H_ */
