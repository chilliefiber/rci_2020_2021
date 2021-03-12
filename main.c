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
	int fd_udp_r, fd_udp_s, fd_tcp_a, fd_tcp_s, fd_tcp_b, fd_tcp_e, newfd, fd_close, maxfd, counter;
    int errcode_udp_r, errcode_udp_s, errcode_tcp_a, errcode_tcp_s, errcode_get;
    int viz_extern = 0, no_backup = 0, extra_com = 0, close_fd = 0;
    struct viz *externo, *backup, **interno;
    unsigned int n_internos;
    
    char buffer_c[128];
   
	fd_set rfds;
    ssize_t nu, nt, nclose;
    socklen_t addrlen_udp_r, addrlen_tcp_a;
    struct addrinfo hints_udp_r, *res_udp_r, hints_udp_s, *res_udp_s, hints_tcp_a, *res_tcp_a, hints_tcp_s, *res_tcp_s;
    struct sockaddr_in addr_udp_r, addr_udp_s, addr_tcp_a;
	struct sigaction act;

    
    /* Protection against SIGPIPE signals */ 
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &act, NULL) == -1) exit(1);
	
	if(argc != 5)
	{
		printf("Wrong format, please run with ./ndn IP TCP regIP regUDP\n");
		exit(1);
	}
	
	helpMenu();
	
	/* UDP ready to receive */
    fd_udp_r=socket(AF_INET,SOCK_DGRAM,0);
    if(fd_udp_r==-1) /*error*/ exit(1);
    
    memset(&hints_udp_r,0,sizeof hints_udp_r);
    hints_udp_r.ai_family=AF_INET;
    hints_udp_r.ai_socktype=SOCK_DGRAM;
    hints_udp_r.ai_flags=AI_PASSIVE;
    
    errcode_udp_r = getaddrinfo(NULL,argv[4],&hints_udp_r,&res_udp_r);
    if(errcode_udp_r!=0) /*error*/ exit(1);
    
	
	/* UDP ready to send */
    fd_udp_s=socket(AF_INET,SOCK_DGRAM,0);
    if(fd_udp_s==-1) /*error*/ exit(1);
    
    memset(&hints_udp_s,0,sizeof hints_udp_s);
    hints_udp_s.ai_family=AF_INET;
    hints_udp_s.ai_socktype=SOCK_DGRAM;
    
    /* TCP ready to accept */
    fd_tcp_a=socket(AF_INET, SOCK_STREAM, 0);
    if(fd_tcp_a==-1) /*error*/ exit(1);
    
    memset(&hints_tcp_a ,0, sizeof hints_tcp_a);
    hints_tcp_a.ai_family=AF_INET;
    hints_tcp_a.ai_socktype=SOCK_STREAM;
    hints_tcp_a.ai_flags=AI_PASSIVE;
    
    errcode_tcp_a = getaddrinfo(NULL, argv[2], &hints_tcp_a, &res_tcp_a);
    if(errcode_tcp_a!=0) /*error*/ exit(1);
    
    nt = bind(fd_tcp_a,res_tcp_a->ai_addr,res_tcp_a->ai_addrlen);
    if(nt==-1) /*error*/ exit(1);
    
    if(listen(fd_tcp_a,5)==-1)    exit(1);
    
    addrlen_tcp_a = sizeof(addr_tcp_a);
    
    /* TCP ready to write */
    fd_tcp_s=socket(AF_INET, SOCK_STREAM, 0);
    if(fd_tcp_s==-1) /*error*/ exit(1);
    
    memset(&hints_tcp_s, 0, sizeof hints_tcp_s);
    hints_tcp_s.ai_family = AF_INET;
    hints_tcp_s.ai_socktype = SOCK_STREAM;
   
    
    /* file descriptors */
    while(1)
    {
		FD_ZERO(&rfds);
        
        /* read from terminal */
        FD_SET(0, &rfds);
    
        /* receive UDP */
        FD_SET(fd_udp_r, &rfds);
        maxfd = max(0, fd_udp_r);
        
        /* accept TCP */
        FD_SET(fd_tcp_a, &rfds);
        maxfd = max(maxfd, fd_tcp_a);
        
        /* read TCP from backup node */
        if(no_backup)
        {
            FD_SET(fd_tcp_b, &rfds);
            maxfd = max(maxfd, fd_tcp_b);
        }
        
        /* read TCP from external neighbour */
        if(viz_extern) 
        {
            FD_SET(fd_tcp_e, &rfds);
            maxfd = max(maxfd, fd_tcp_e);
        }
        
        /* read TCP from new communication */
        if(extra_com) 
        {
            FD_SET(newfd, &rfds);
            maxfd = max(maxfd, newfd);
        }

		/* if there is a link to be closed then this fd will read 0 and then close it */
        if(close_fd) 
        {
            FD_SET(fd_close, &rfds);
            maxfd = max(maxfd, fd_close);
        }
        
        /* select upon which file descriptor to act */
        counter = select(maxfd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        if(counter<=0)  exit(1);
		
		/* here we'll read a line from terminal and then process it accordingly */
        if(FD_ISSET(0, &rfds)) 
        {
			
		
		}
		
		
		/* here a UDP message will be received and then processed */
        if(FD_ISSET(fd_udp_r, &rfds)) 
        {
		
			
		}
		
		
		/* here we'll accept a request to connect */
        if(FD_ISSET(fd_tcp_a, &rfds)) 
        {
			newfd = accept(fd_tcp_a, (struct sockaddr*)&addr_tcp_a, &addrlen_tcp_a);
            if(newfd==-1) /*error*/ exit(1);
            extra_com=1;	
		}	
		
		/* here we'll close a link that as been closed on the other side */
        if(close_fd) 
        {
            if(FD_ISSET(fd_close, &rfds)) 
            {
                nclose=read(fd_close, buffer_c, 128);
                if(nclose == 0) 
                {
                    close(fd_close);
                    close_fd = 0;
                }
            }
        }
        
         /* if there's a backup node, here we'll read any messages it sends */
        if(no_backup)
        {
			
		}
		
		/* if there's a new link it's handled here, then if need be they're assigned to extern or backup node*/
        if(extra_com) 
        {
			
		}
		
		/* if there's a external node, here we'll read any messages it sends */
        if(viz_extern) 
        {
			
		}
		/* ... */ 
		
	}
   
    freeaddrinfo(res_udp_r);
    freeaddrinfo(res_tcp_a);
    close(fd_tcp_a);
    
    return 0;
}
