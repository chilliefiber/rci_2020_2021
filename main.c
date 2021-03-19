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


#define N 2 //capacidade da cache de objetos do nó

typedef struct object{
		char *subname;
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

typedef struct node_list{
    char node_IP[INET_ADDRSTRLEN]; //endereço IP do nó	
    char node_port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;

typedef struct list_objects{
	char *objct;
	struct list_objects *next;
}list_objects;

list_objects *createinsertObject(list_objects *head, char *subname, char *str_id)
{
    list_objects *tmp = head;
    list_objects *new_obj = safeMalloc(sizeof(list_objects));
    new_obj->objct = safeMalloc(strlen(str_id)+strlen(subname)+1);
	
    strcpy(new_obj->objct, str_id);
    strcat(new_obj->objct, subname);

    if(head == NULL)
    {
	head = new_obj;
	new_obj->next = NULL;
    }
    else
    {
	 while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = new_obj;
        new_obj->next = NULL;
    }
	
    return head;
}

node_list *parseNodelist(char* datagram, int *num_nodes)
{
    int datagram_ix = 0, line_ix=0;
    node_list* list = NULL;
    *num_nodes = -1;
    char c = datagram[datagram_ix];
    char line[150]; // colocar aqui uma variavel com #DEFINE 150
    while (c != '\0')
    {
        if (c == '\n'){
            (*num_nodes)++;
            // neste caso ou é zero, e portanto lemos apenas o comando
            // ou devemos fazer um elemento da lista de nós
            if (*num_nodes)
            {   
                line[line_ix] = '\0';
                parseLine(&list, line);
                line_ix = 0; // reiniciar o indice, porque vamos começar a ler uma linha nova

            }
        }
        // quando num_nodes == -1 estamos a ler o comando e nao um part IP/porto
        else if (*num_nodes != -1)
        {
            line[line_ix] = c;
            line_ix++;
        }
        datagram_ix++;
        c = datagram[datagram_ix];
    }
    return list;
}

void parseLine(node_list **list, char *line)
{
    node_list *this = safeMalloc(sizeof(node_list));
    this.next = NULL;
    node_list *aux = *list;
    sscanf(line,"%s %s", this.node_IP, this.node_port);
    if (*list == NULL)
        *list = this;
    else
    {
        // go to last element of list
        for (aux; aux->next != NULL; aux = aux->next);
        // place newly created element in list
        aux->next = this;
    }
}
// enum dos vários estados associados à rede de nós
// NONODES no caso em que não existem nós
// TWONODES no caso em que a rede tem dois nós
// MANYNODES no caso em que a rede tem mais que dois nós
enum state {NONODES, TWONODES, MANYNODES};

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
        enum {not_waiting, waiting_for_list, waiting_for_regok, waiting_for_unregok} udp_state;
        socklen_t addrlen;
        no self;
	
	list_objects *head = NULL;
	char *str_id;
	
	struct sigaction act;
	// Protection against SIGPIPE signals 
	memset(&act, 0, sizeof act);
	act.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
	
	if(argc != 5 || isIP(argv[1]) == 0 || isPort(argv[2]) == 0 || isIP(argv[3]) == 0 || isPort(argv[4]) == 0)
	{
		printf("Usage: ./ndn IP TCP regIP regUDP\n");
		exit(1);
	}
	
	if ((fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) exit(1);

        memset(&hints, 0, sizeof hints);
        hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_DGRAM;
	
	errcode_udp = getaddrinfo(argv[3],argv[4],&hints,&res);
    	if(errcode_udp!=0)  
	{
	   printf("Error getting address information for UDP server socket\n");
	   exit(1);
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
			if (udp_state == not_waiting)
                        {
                            printf("WARNING - Received trash through UDP\n");
                            addrlen = sizeof(addr);
                            n = recvfrom(fd, udp_buffer, 999, 0, &addr, &addrlen);
                            if (n == -1)
                            {
                                printf("Error in recvfrom!\n");
                                exit(-1);
                            }
                            udp_buffer[n] = '\0';
                            printf("This is the trash\n");
                            printf("%s", udp_buffer);
                            printf("\n That was it\n");
                        }
                        else if (udp_state == waiting_for_regok)
                        {
                            addrlen = sizeof(addr);
                            n = recvfrom(fd, udp_buffer, 999, 0, &addr, &addrlen);
                            if (n == -1)
                            {
                                printf("Error in recvfrom!\n");
                                exit(-1);
                            }
                            udp_buffer[n] = '\0';
                            if (!strcmp(udp_buffer, "OKREG")){
                                printf("We received the confirmation of registration from the server\n");
                                udp_state = not_waiting;
                            }
                            else
                            {                       
                                printf("WARNING - Received trash through UDP\n");
                                printf("This is the trash\n");
                                printf("%s", udp_buffer);
                                printf("\n That was it\n");
                            }
                        }
                        else if (udp_state == waiting_for_unregok)
                        {
                            addrlen = sizeof(addr);
                            n = recvfrom(fd, udp_buffer, 999, 0, &addr, &addrlen);
                            if (n == -1)
                            {
                                printf("Error in recvfrom!\n");
                                exit(-1);
                            }
                            udp_buffer[n] = '\0';
                            if (!strcmp(udp_buffer, "OKUNREG")) 
                            {
                                printf("We received the confirmation of unregistration from the server\n");
                                udp_state = not_waiting;   
                            }
                            else
                            {
                                printf("WARNING - Received trash through UDP\n");
                                printf("This is the trash\n");
                                printf("%s", udp_buffer);
                                printf("\n That was it\n");
                            }
                        }
                        else if (udp_state == waiting_for_list)
                        {
                            
                            addrlen = sizeof(addr);
                            n = recvfrom(fd, udp_buffer, 999, 0, &addr, &addrlen);
                            if (n == -1)
                            {
                                printf("Error in recvfrom!\n");
                                exit(-1);
                            }
                            udp_buffer[n] = '\0';
                            if (!strcmp(udp_buffer, "NODESLIST")) 
                            {
                                printf("We received the list of nodes from the server\n");
                                // assign a random node 

                                udp_state = not_waiting;   
                            }
                            else
                            {
                                printf("WARNING - Received trash through UDP\n");
                                printf("This is the trash\n");
                                printf("%s", udp_buffer);
                                printf("\n That was it\n");
                            }
                        }
		}
		
		 // Ler input do utilizador no terminal
    		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
		    user_input = readCommand(&instr_code);
		    if (instr_code == JOIN_ID && state == NONODES)
		    {
			if(sscanf(user_input,"%d %d",&self.net,&self.id) != 2)
			{
		            printf("Error in sscanf JOIN_ID\n");
			    exit(1);
			}
			strcpy(self.node_IP, argv[1]);
			strcpy(self.node_port, argv[2]);
			str_id = safeMalloc(sizeof(self.id)+1);
			sprintf(str_id,"%d.",self.id);
			    
			_state = TWONODES;
                    }
		    
		    if (instr_code == CREATE && _state != NONODES)
		    {
			    
		    	head = createinsertObject(head,user_input,str_id);
		    }
			
		    free(user_input);
		}
		
		
		
	}	
	
	return 0;
}

