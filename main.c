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
		int id_obj;
}object;

typedef struct no{
	object *conj_objects; //conjunto de objetos nomeados contidos num nó
	int id;    //identificador do nó
	char node_IP[INET_ADDRSTRLEN]; //endereço IP do nó
    	char node_port[NI_MAXSERV]; //Porto TCP do nó
    	struct viz *externo; //Ponteiro para vizinho externo
    	struct viz *backup; //Ponteiro para vizinho de recuperação
}no;

typedef struct viz{
	int fd; //file descriptor 
	int id;    //identificador do vizinho
	char viz_IP[INET_ADDRSTRLEN]; //endereço IP do vizinho
    	char viz_port[NI_MAXSERV]; //Porto TCP do vizinho
}viz;

//lista dos vizinhos internos do nó
typedef struct internals{
	struct viz *this;
	struct viz *next;
}internals;
