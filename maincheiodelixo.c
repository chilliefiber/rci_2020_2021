#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#include "input.h"


#define max(A,B) ((A)>=(B)?(A):(B))
#define STRING_SIZE 100

typedef struct object{
		char subname[STRING_SIZE];
}object;

typedef struct node{
	object *ident; //conjunto de objetos nomeados contidos num nó
	int id;    //identificador do nó
	char node_IP[INET_ADDRSTRLEN]; //endereço IP do nó
    char node_port[NI_MAXSERV]; //Porto TCP do nó
}no;

typedef struct viz{
	//char *externo;
	int id;    //identificador do nó
	char node_IP[INET_ADDRSTRLEN]; //endereço IP do vizinho
    char node_port[NI_MAXSERV]; //Porto TCP do vizinho
}viz;

//lista dos vizinhos internos do nó
typedef struct internals{
	struct viz *this;
	struct viz *next;
}internals;


int main(int argc, char *argv[])
{
    fd_set rfds;
    enum instr instr_code;
    struct addrinfo hints, *res;
    struct sockaddr_in addr; 
    struct sigaction act;

    return 0;
}
