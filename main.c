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

int main(int argc, char *argv[])
{
	fd_set rfds;
	int fd_tcp, fd_udp, maxfd, counter;
	struct addrinfo hints, *res;
  	struct sockaddr_in addr;
	enum instr instr_code;
	char *user_input;
	cache_objects cache[N];
	
	struct sigaction act;
	/* Protection against SIGPIPE signals */ 
	memset(&act, 0, sizeof act);
	act.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
	
	if(argc != 5 || isIP(argv[1]) == 0 || isPort(argv[2]) == 0 || isIP(argv[3]) == 0 || isPort(argv[4]) == 0)
	{
		printf("Usage: ./ndn IP TCP regIP regUDP\n");
		exit(1);
	}
	
	helpMenu();
	
	while(1)
	{
		/*initialize file descriptor set*/
		FD_ZERO(&rfds);
		
		FD_SET(fd_tcp, &rfds);
    
		FD_SET(fd_udp, &rfds);
		
    		FD_SET(STDIN_FILENO, &rfds);
    
		max_fd=max(fd_tcp, fd_udp);
    		max_fd=max(max_fd, STDIN_FILENO);
		
		 /* read TCP from an internal neighbour */
        	if (interno_on) 
		{
            	  FD_SET(fd_tcp_i, &rfds);
            	  maxfd = max(maxfd, fd_tcp_i);
        	}
        
        	/* read TCP from external neighbour */
        	if (externo_on) 
		{
            	  FD_SET(fd_tcp_e, &rfds);
                  maxfd = max(maxfd, fd_tcp_e);
                }
		
		/* select upon which file descriptor to act */
        	counter = select(maxfd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        	if(counter<=0)  exit(1);
		
		
		// canal onde o próprio nó recebe chamadas TCP
		if (FD_ISSET(fd_tcp, &rfds))
		{
			
		}
		
		// UDP
		if (FD_ISSET(fd_udp, &rfds))
		{
			
		}
		
		 // Ler input do utilizador no terminal
    		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
		   user_input = readCommand(&instr_code);
			
			
		}
		
		/* here we'll accept a request to connect */
        	if (FD_ISSET(fd_tcp, &rfds)) 
		{
            		
		}
		
		// Ler dum vizinho interno
		if (interno_on)
		{
		
		}
		
		// Ler do vizinho externo 
		if (externo_on)
		{
		
		}
	}	
	
	return 0;
}

