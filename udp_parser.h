#ifndef UDP_PARSER_H
#define UDP_PARSER_H

#include <arpa/inet.h>
#include <netdb.h>

typedef struct node_list{
    char node_IP[INET_ADDRSTRLEN]; //endereço IP do nó	
    char node_port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;

void parseNodeListRecursive(char* datagram, int *num_nodes, node_list **list);
char* isNodesList(char* datagram, unsigned int net, char *nodeslist_received);
#endif