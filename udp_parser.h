#ifndef UDP_PARSER_H
#define UDP_PARSER_H

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include "nodes.h"

typedef struct node_list{
    char IP[NI_MAXHOST]; //endereço IP do nó	
    char port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;
/**
 *
 *
 *\return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK e NO_ERROR
 */
int parseNodeListRecursive(char* datagram, node_list **list, int *num_nodes);


/**
 *
 * arg error_flag: indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK e NO_ERROR  
 */
char* isNodesList(char* datagram, char *net, char *nodeslist_received, int *error_flag);
void freeNodeList(node_list **list);

/**
 *
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, TIMER_EXPIRED, OK_READ 
 */
int sendAndWait(int fd_udp, struct timeval *tv, char* send_error_msg, char *send_IP, char *send_UDP, char *dgram);

/**
 * 
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, DGRAM_RECEIVED
 */
int getDgram(char *send_error_msg, char *send_IP, char *send_UDP, char *dgram);

/**
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int unreg(no *self, char *send_IP, char *send_UDP);

/**
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int getNodesList(char *self_net, char *send_IP, char *send_UDP, char *dgram);

/**
 * \return inteiro indicativo de um código de erro: ERROR, END_EXECUTION, NO_ERROR
 */
int setListedNodeAsExternal(viz *external, node_list *aux_list_of_nodes, no *self, char *message_buffer);

/**
 * \return inteiro indicativo de um código de erro: ERROR, END_EXECUTION, NO_ERROR
 */
int chooseExternalFromList(node_list *list_of_nodes, int num_nodes, viz *external, no *self, char *message_buffer);

/**
 * \return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK, ERROR, NO_ERROR  
 */
int setupExternal(char *list_msg, viz **external, no *self);

/**
 * \return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int reg(no *self, char *send_IP, char *send_UDP);

#endif
