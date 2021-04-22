#ifndef UDP_PARSER_H
#define UDP_PARSER_H

#include <arpa/inet.h>
#include <netdb.h>

typedef struct node_list{
    char IP[NI_MAXHOST]; //endereço IP do nó	
    char port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;

int parseNodeListRecursive(char* datagram, node_list **list, int *num_nodes);
char* isNodesList(char* datagram, char *net, char *nodeslist_received);
void freeNodeList(node_list **list);
#endif
