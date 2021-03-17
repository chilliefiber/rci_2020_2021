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

#define N 2 //capacidade da cache de objetos do nó

typedef struct object{
		char subname[STRING_SIZE];
		int id_obj;
}object;

typedef struct cache_objects{
		object obj;
}cache_objects;

typedef struct no{
	int net; //identificador da rede
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


// enum dos vários estados associados à rede de nós
// EMPTY no caso em que não existem nós
// ONENODE no caso em que a rede tem um só nó,
// TWONODES no caso em que a rede tem dois nós
// MANYNODES no caso em que a rede tem mais que dois nós
enum state {EMPTY, ONENODE, TWONODES, MANYNODES};

int main(int argc, char *argv[])
{
	fd_set rfds;
	int fd_udp, maxfd, counter;
	int errcode_udp;
	struct addrinfo hints, *res;
  	struct sockaddr_in addr;
	enum instr instr_code;
	char *user_input;
	cache_objects cache[N];
	
	struct sigaction act;
	// Protection against SIGPIPE signals 
	memset(&act, 0, sizeof act);
	act.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
	
	if(argc != 5 || isIP(argv[1]) == 0 || isPort(argv[2]) == 0 || strcmp("193.136.138.142", argv[3]) != 0 || strcmp("59000", argv[4]) != 0))
	{
		printf("Usage: ./ndn IP TCP regIP regUDP\n");
		exit(1);
	}
	
	if ((fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) exit(1);

        memset(&hints, 0, sizeof hints);
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_DGRAM;
        hints.ai_flags=AI_PASSIVE;
	
	errcode_udp = getaddrinfo(NULL,argv[4],&hints,&res);
    	if(errcode_udp!=0)  
	{
	   printf("Error getting address information for UDP server socket\n");
	   exit(1);
	}	
    
    	if(bind(fd_udp,res->ai_addr, res->ai_addrlen) == -1)
	{
	   printf("Error binding to UDP server socket\n");
           freeaddrinfo(res);	
	}
	
	freeaddrinfo(res);
	
	
	helpMenu();
	
	while(1)
	{
		// initialize file descriptor set
		FD_ZERO(&rfds);
    
		FD_SET(fd_udp, &rfds);
		
    		FD_SET(STDIN_FILENO, &rfds);
    
		max_fd=max(STDIN_FILENO, fd_udp);
		
		
		
		// select upon which file descriptor to act 
        	counter = select(maxfd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        	if(counter<=0)  exit(1);
		
		
		
		// UDP
		if (FD_ISSET(fd_udp, &rfds))
		{
			
		}
		
		 // Ler input do utilizador no terminal
    		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
		   user_input = readCommand(&instr_code);
			 
			
		   free(user_input);
		}
		
		
		
	}	
	
	return 0;
}

