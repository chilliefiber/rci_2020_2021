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
#include "udp_parser.h"
#include "errcheck.h"
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
    unsigned int net; //identificador da rede
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


typedef struct list_objects{
    char *objct;
    struct list_objects *next;
}list_objects;

list_objects *createinsertObject(list_objects *head, char *subname, char *str_id)
{
    list_objects *tmp = head;
    list_objects *new_obj = safeMalloc(sizeof(list_objects));
    new_obj->objct = safeMalloc(strlen(str_id)+strlen(subname)+1);
// INSEGURO
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

int main(int argc, char *argv[])
{
    // enum dos vários estados associados à rede de nós
    // NONODES no caso em que não existem nós
    // TWONODES no caso em que a rede tem dois nós
    // MANYNODES no caso em que a rede tem mais que dois nós
    enum {NONODES, TWONODES, MANYNODES} state;
    state = NONODES;
    node_list *nodes_fucking_list;
    fd_set rfds;
    int num_nodes, fd_udp, max_fd, counter;
    int errcode;
    enum instr instr_code;
    char *user_input, flag;
    char message_buffer[150], dgram[1000], *list_msg;
    //cache_objects cache[N];
    enum {not_waiting, waiting_for_list, waiting_for_regok, waiting_for_unregok} udp_state;
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
    helpMenu();
    while(1)
    {
        // initialize file descriptor set
        FD_ZERO(&rfds);
        FD_SET(fd_udp, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        max_fd=max(STDIN_FILENO, fd_udp);
        // select upon which file descriptor to act 
        counter = select(max_fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        if(counter<=0)  exit(1);
        // UDP
        if (FD_ISSET(fd_udp, &rfds))
        {
            safeRecvFrom(fd_udp, dgram, 999);
            if (udp_state == not_waiting)
                warnOfTrashReceived("WARNING - Received trash through UDP: udp_state not_waiting\n", dgram);
            else if (udp_state == waiting_for_regok)
            {
                if (!strcmp(dgram, "OKREG")){
                    printf("We received the confirmation of registration from the server\n");
                    udp_state = not_waiting;
                    state = TWONODES; //isto so para ser diferente de NONODESde momento
                }
                else
                    warnOfTrashReceived("WARNING - Received trash through UDP: udp_state waiting_for_regok\n", dgram);
            }
            else if (udp_state == waiting_for_unregok)
            {
                if (!strcmp(dgram, "OKUNREG")) 
                {
                    printf("We received the confirmation of unregistration from the server\n");
                    udp_state = not_waiting;   
                    state = NONODES;
                }
                else
                    warnOfTrashReceived("WARNING - Received trash through UDP: udp_state waiting_for_unregok\n", dgram);
            }
            else if (udp_state == waiting_for_list)
            { 
                // fazer função que verifica se temos uma lista de nós
                list_msg = isNodesList(dgram, self.net, &flag);
                // neste caso não recebemos
                if (flag)
                {
                    printf("We received the list of nodes from the server\n");
                    // assign a random node 
                    if (list_msg)
                    {
                        num_nodes = 0;
                        // aqui, é preciso escrever código para limpar 
                        // a memória da lista anterior, caso ela exista
                        // na verdade, o correto será apagar a lista
                        // imediatamente após a confirmação de que entrámos no servidor
                        // e portanto não precisamos mais dela
                        nodes_fucking_list = NULL;
                        parseNodeListRecursive(list_msg, &num_nodes, &nodes_fucking_list);
                    }
                    //write udp
                    // criar string para enviar o registo do nó
                    errcode = snprintf(message_buffer, 150, "REG %u %s %s", self.net, self.node_IP, self.node_port);  
                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                    {
                        fprintf(stderr, "error in REG UDP message creation: %s\n", strerror(errno));
                        exit(-1);
                    }
                    sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n");
                    udp_state = waiting_for_regok;
                }
                else
                    warnOfTrashReceived("WARNING - Received trash through UDP: udp_state waiting_for_list\n", dgram);
            }
        }
        // Ler input do utilizador no terminal
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            user_input = readCommand(&instr_code);
            if (instr_code == JOIN_ID && state == NONODES)
            {
                if(sscanf(user_input,"%u %d",&self.net,&self.id) != 2)
                {
                    printf("Error in sscanf JOIN_ID\n");
                    exit(1);
                }
                // MAIS OVERFLOWS
                strcpy(self.node_IP, argv[1]);
                strcpy(self.node_port, argv[2]);
                str_id = safeMalloc(sizeof(self.id)+1);
                // bad monkey no banana
                // passar isto para snprintf
                // isto é um buffer overflow
                sprintf(str_id,"%d.",self.id);
                // criar string para enviar o pedido de nós
                errcode = snprintf(message_buffer, 150, "NODES %u", self.net);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 100)
                {
                    fprintf(stderr, "error in JOIN UDP message creation: %s\n", strerror(errno));
                    exit(-1);
                }
                sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in JOIN UDP message send\n");
                udp_state = waiting_for_list;
            }
            else if (instr_code == LEAVE && state != NONODES)
            {
                // criar string para enviar o desregisto (?isto é uma palavra) do nó
                errcode = snprintf(message_buffer, 150, "UNREG %u %s %s", self.net, self.node_IP, self.node_port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in UNREG UDP message creation: %s\n", strerror(errno));
                    exit(-1);
                }
                sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in UNREG UDP message send\n");
                udp_state = waiting_for_unregok;
            }
            else if (instr_code == CREATE && state != NONODES)
                head = createinsertObject(head,user_input,str_id);
            else if(instr_code == EXIT)
                exit(0);
            free(user_input);
        }
    }	
    return 0;
}
