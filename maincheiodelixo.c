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

int start_node(struct no *node, struct viz *backup, struct viz *viz_externo, int id, char *node_IP, char *node_port);
no *start_net(struct no *first, char **argv, int id);

int main(int argc, char *argv[])
{
    int maxfd, newfd, fd_udp_r, fd_udp_s, fd_tcp_a, fd_tcp_i, fd_tcp_e, fd_get, fdclose;
    int counter, errcode_udp_r, errcode_udp_s, errcode_tcp_a, errcode_tcp_e, errcode_get;
    int externo_on = 0, interno_on = 0, extra_com = 0, close_fd = 0;
    char *input;	
    enum instr instr_code;
	
    fd_set rfds;
    ssize_t nu, nclose;
    socklen_t addrlen_udp_r, addrlen_udp_s, addrlen_tcp_a;
    struct addrinfo hints_udp_r, *res_udp_r, hints_udp_s, *res_udp_s, hints_tcp_a, *res_tcp_a, hints_tcp_e, *res_tcp_e, hints_get, *res_get;
    struct sockaddr_in addr_udp_r, addr_udp_s, addr_tcp_a;
	
    struct no *local=NULL;

    struct sigaction act;
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
	
    if(argc != 5 || isIP(argv[1]) == 0 || isPort(argv[2]) == 0 || isIP(argv[3]) == 0 || isPort(argv[4]) == 0)
    {
	printf("Wrong format, please run with ./ndn IP TCP regIP regUDP\n");
	exit(1);
    }
	
	
	
    
    /* UDP ready to receive */
    fd_udp_r=socket(AF_INET,SOCK_DGRAM,0);
    if(fd_udp_r==-1)  exit(1);
    
    memset(&hints_udp_r,0,sizeof hints_udp_r);
    hints_udp_r.ai_family=AF_INET;
    hints_udp_r.ai_socktype=SOCK_DGRAM;
    hints_udp_r.ai_flags=AI_PASSIVE;
    
    errcode_udp_r = getaddrinfo(argv[3],argv[4],&hints_udp_r,&res_udp_r);
    if(errcode_udp_r!=0)  exit(1);
    
    nu = bind(fdur,res_udp_r->ai_addr, res_udp_r->ai_addrlen);
    if(nu==-1)   exit(1);
    
    addrlen_udp_r=sizeof(addr_udp_r);
    
    /* UDP ready to send */
    fd_udp_s=socket(AF_INET,SOCK_DGRAM,0);
    if(fd_udp_s==-1)  exit(1);
    
    if (setsockopt(fd_udp_s, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer))<0) {
        perror("Error");
        exit(1);
    }
    
    memset(&hints_udp_s,0,sizeof hints_udp_s);
    hints_udp_s.ai_family=AF_INET;
    hints_udp_s.ai_socktype=SOCK_DGRAM;
    
    /* TCP ready to accept */
    fd_tcp_a=socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp_a==-1) exit(1);
    
    memset(&hints_tcp_a ,0, sizeof hints_tcp_a);
    hints_tcp_a.ai_family=AF_INET;
    hints_tcp_a.ai_socktype=SOCK_STREAM;
    hints_tcp_a.ai_flags=AI_PASSIVE;
    
    errcode_tcp_a = getaddrinfo(NULL, argv[2], &hints_tcp_a, &res_tcp_a);
    if(errcode_tcp_a!=0)    exit(1);
    
    nt = bind(fd_tcp_a,res_tcp_a->ai_addr,res_tcp_a->ai_addrlen);
    if(nt==-1)  exit(1);
    
    if(listen(fd_tcp_a,5)==-1)    exit(1);
    
    addrlen_tcp_a = sizeof(addr_tcp_a);
    
    /* TCP ready to write */
    fd_tcp_e=socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp_e==-1) exit(1);
    
    memset(&hints_tcp_e, 0, sizeof hints_tcp_e);
    hints_tcp_e.ai_family = AF_INET;
    hints_tcp_e.ai_socktype = SOCK_STREAM;

    helpMenu();
	
    /* file descriptors */
    while(1){
	/* clean the file descriptors */
        FD_ZERO(&rfds);
        
        /* read from terminal */
        FD_SET(0, &rfds);
    
        /* receive UDP */
        FD_SET(fd_udp_r, &rfds);
        maxfd = max(0, fd_udp_r);
        
        /* accept TCP */
        FD_SET(fd_tcp_a, &rfds);
        maxfd = max(maxfd, fd_tcp_a);
        
        /* read TCP from internal neighbour */
        if (interno_on) {
            FD_SET(fd_tcp_b, &rfds);
            maxfd = max(maxfd, fd_tcp_b);
        }
        
        /* read TCP from external neighbour */
        if (externo_on) {
            FD_SET(fd_tcp_e, &rfds);
            maxfd = max(maxfd, fd_tcp_e);
        }
        
        /* read TCP from new communication */
        if (extra_com) {
            FD_SET(newfd, &rfds);
            maxfd = max(maxfd, newfd);
        }
        
        /* if there is a link to be closed then this fd will read 0 and then close it */
        if (close_fd) {
            FD_SET(fdclose, &rfds);
            maxfd = max(maxfd, fdclose);
        }
        
        /* select upon which file descriptor to act */
        counter = select(maxfd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        if(counter<=0)  exit(1);
        
        /* here we'll read a line from terminal and then process it accordingly */
        if (FD_ISSET(0, &rfds)) 
	{
	     input = readCommand(&instr_code); 
		
		
	}
    return 0;
}

int start_node(struct no *node, struct viz *backup, struct viz *viz_externo, int id, char *node_IP, char *node_port)
{
	node->id = id;
	strcpy(node->node_IP, node_IP);
   	strcpy(node->node_port, node_port);
    	node->externo = viz_externo;
    	node->backup = backup;
	
	node->conj_objects = (object *)malloc(N*sizeof(object));  //dps fazer verificação
	
	return 0;
}

no *start_net(struct no *first, char **argv, int id)
{
	first=(struct no *)calloc(1,sizeof(struct no));
	start_node(first, NULL, NULL, id, argv[1], argv[2]);
	return first;
}
