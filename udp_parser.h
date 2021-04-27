#ifndef UDP_PARSER_H
#define UDP_PARSER_H

#define REG 0
#define UNREG 1
#define NODES 2

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include "nodes.h"

typedef struct node_list{
    char IP[NI_MAXHOST]; //endereço IP do nó	
    char port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;

int parseNodeListRecursive(char* datagram, node_list **list, int *num_nodes);
char* isNodesList(char* datagram, char *net, char *nodeslist_received, int *error_flag);
void freeNodeList(node_list **list);
int sendAndWait(int fd_udp, struct timeval *tv, char* send_error_msg, char *send_IP, char *send_UDP, char *dgram);
int getDgram(char *send_error_msg, char *send_IP, char *send_UDP, char *dgram);
int unreg(no *self, char *send_IP, char *send_UDP);
int getNodesList();
int setListedNodeAsExternal(viz *external, node_list *aux_list_of_nodes, no *self);
int chooseExternalFromList();
int setupExternal();
int reg();

#endif
