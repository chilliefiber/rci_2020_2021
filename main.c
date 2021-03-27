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
#include "tcp.h"
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
    char IP[NI_MAXHOST]; //endereço IP do nó
    char port[NI_MAXSERV]; //Porto TCP do nó
    struct viz *externo; //Ponteiro para vizinho externo
    struct viz *backup; //Ponteiro para vizinho de recuperação
}no;


//lista dos vizinhos internos do nó
typedef struct internals{
    struct viz *this;
    struct internals *next;
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

void addToList(internals *int_neighbours, viz *new)
{
    internals *aux = int_neighbours;
    int_neighbours = safeMalloc(sizeof(internals));
    int_neighbours->this = new;
    int_neighbours->next = aux;
}

int main(int argc, char *argv[])
{
    // enum dos vários estados associados à rede de nós
    // NONODES no caso em que não existem nós
    // TWONODES no caso em que a rede tem dois nós
    // MANYNODES no caso em que a rede tem mais que dois nós
    enum {NONODES, ONENODE, TWONODES, MANYNODES} network_state;
    network_state = NONODES;
    node_list *nodes_fucking_list;
    fd_set rfds;
    int num_nodes, fd_udp, max_fd, counter, tcp_server_fd;
    int errcode;
    enum instr instr_code;
    char *user_input, flag, tcp_read_flag;
    char command[150], arg1[150], arg2[150];
    int word_count;
    char message_buffer[150], dgram[1000], *list_msg;
    struct addrinfo hints, *res;
    socklen_t addrlen;
    struct sockaddr addr;
    // potencialmente não será o vizinho externo
    // pode ser apenas um potencial vizinho externo,
    // no caso em que fazemos JOIN, fizemos ligação
    // mas ainda não recebemos a mensagem de contacto
    viz *external=NULL, *new=NULL, *backup=safeMalloc(sizeof(viz)); 
    // lista de vizinhos internos
    internals *int_neighbours = NULL, *neigh_aux;
    //cache_objects cache[N];
    // estados associados ao select
    enum {not_waiting, waiting_for_list, waiting_for_regok, waiting_for_unregok} udp_state;
    udp_state = not_waiting;
    enum {waiting_for_backup} tcp_state;
    // lista de mensagens recebidas num readTCP
    messages *msg_list, *msg_aux;
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
    safeTCPSocket(&tcp_server_fd);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    safeGetAddrInfo(NULL, argv[2], &hints, &res, "Error getting address info for TCP server socket\n");
    if (bind(tcp_server_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "Error binding TCP server: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (listen(tcp_server_fd, 5) == -1)
    {
        fprintf(stderr, "Error putting TCP server to listen: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    helpMenu();
    // este while precisa do for interno para lidar com vários fds set ao mesmo tempo
    // ver slide do guia 
    while(1)
    {
        // initialize file descriptor set
        FD_ZERO(&rfds);

        FD_SET(fd_udp, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        max_fd=max(STDIN_FILENO, fd_udp);
        // se o processo não está em nenhuma rede, não é suposto contactarem nos por TCP
        FD_SET(tcp_server_fd, &rfds); 
        max_fd = max(max_fd, tcp_server_fd);
        if (external)
        {
            FD_SET(external->fd, &rfds);
            max_fd = max(max_fd, external->fd);
        }
        neigh_aux = int_neighbours;
        while (neigh_aux != NULL)
        {
            FD_SET(neigh_aux->this->fd, &rfds);
            max_fd = max(max_fd, neigh_aux->this->fd);
            neigh_aux = neigh_aux->next;
        }
        // select upon which file descriptor to act 
        counter = select(max_fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        if(counter<=0)  exit(1);
        // TCP
        if (FD_ISSET(tcp_server_fd, &rfds))
        {
            new = safeMalloc(sizeof(viz));
            new->next_av_ix = 0;
            addrlen = sizeof(addr);
            if ((new->fd = accept(tcp_server_fd, &addr, &addrlen)) == -1)
            {
                fprintf(stderr, "Error accepting new TCP connection: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            // se este processo estava em modo NONODES, ninguém nos deve contactar por TCP
            // porque não estamos ligados a qualquer rede
            if (network_state == NONODES)
            {
                close(new->fd);
                free(new);
                new = NULL;
            }
            // se este processo estava em modo ONENODE, o primeiro tipo que nos contactar
            // será o nosso vizinho externo
            // Neste caso, apenas aceitamos a conexão e estabelecemos o fd do vizinho externo.
            // Ficamos à espera que ele nos envie o NEW para lhe devolvermos o EXTERN
            else if (network_state == ONENODE)
            {
                external = new;
                new = NULL;
            } 
            // se este processo estava em modo TWONODES ou MANYNODES, alguém que nos contacta
            // será mais um vizinho interno
            else if (network_state == TWONODES || network_state == MANYNODES)
            {
                errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    // isto tá mal, o strncpy não afeta o errno!!
                    // deixo por agora para me lembrar de mudar em todos
                    fprintf(stderr, "error in EXTERN message creation when there are only two nodes: %s\n", strerror(errno));
                    exit(-1);
                }
                writeTCP(new->fd, strlen(message_buffer), message_buffer);
                network_state = MANYNODES;
                addToList(int_neighbours, new);
                new = NULL; // possivelmente passar esta linha de código para o fim de todas as condições
            }
            // não é suposto acontecer alguma vez estas 2 condições
            // isto está aqui apenas por motivos de debug, caso haja alguma falha na máquina de estados
            else
                printf("Error in finite state machine: network_state CODE 1\n");
            if (new != NULL)
            {
                printf("Error in finite state machine: network_state CODE 2\n");
                free(new);
                new = NULL;
            }
        }
        if (external && FD_ISSET(external->fd, &rfds))
        {
            // nesta altura podemos receber ou o EXTERN ou o NEW
            if ((tcp_read_flag = readTCP(external)) == MSG_FINISH)
            {
                msg_list = processReadTCP(external, 0);
                while (msg_list != NULL)
                {
                    word_count = sscanf(msg_list->message, "%s %s %s\n", command, arg1, arg2);
                    // adicionar aqui a verificação do sscanf e errno e assim

                    // este é o caso em que nós fizemos connect e enviámos o NEW
                    // vamos armazenar a informação do nosso backup e registar os nossos
                    // dados no servidor de nós
                    if (!strcmp(command, "EXTERN") && word_count == 3 && tcp_state == waiting_for_backup)
                    {
                        printf("Just received the backup data\n");
                        strncpy(backup->IP, arg1, NI_MAXHOST);
                        strncpy(backup->port, arg2, NI_MAXSERV);

                        // criar string para enviar o registo do nó
                        errcode = snprintf(message_buffer, 150, "REG %u %s %s", self.net, self.IP, self.port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in REG UDP message creation: %s\n", strerror(errno));
                            exit(-1);
                        }
                        sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n");
                        udp_state = waiting_for_regok;
                        // deviamos colocar aqui o two nodes/many nodes
                    }
                    // este é o caso em que recebemos connect, mas 
                    // estávamos sozinhos na rede, então o nosso vizinho externo foi
                    // o tipo que fez o connect e ele envia-nos o NEW. Com a informação
                    // recebida no new, vamos poder retribuir o EXTERN
                    if (!strcmp(command, "NEW") && word_count == 3)
                    {
                        printf("Just received our newly arrived external's data\n");
                        strncpy(external->IP, arg1, NI_MAXHOST);
                        strncpy(external->port, arg2, NI_MAXSERV);

                        errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            // isto tá mal, o strncpy não afeta o errno!!
                            // deixo por agora para me lembrar de mudar em todos
                            fprintf(stderr, "error in EXTERN message creation when there are only two nodes: %s\n", strerror(errno));
                            exit(-1);
                        }
                        writeTCP(external->fd, strlen(message_buffer), message_buffer);
                        network_state = TWONODES;
                    }
                    msg_aux = msg_list;
                    msg_list = msg_list->next;
                    free(msg_aux->message)
                    free(msg_aux);
                    msg_aux = NULL;
                }
            }
        }
        // mudar isto tudo
        neigh_aux = int_neighbours;
        while (neigh_aux != NULL)
        {
            // de momento de um vizinho interno só recebemos o new
            if (FD_ISSET(neigh_aux->this->fd, &rfds))
            {
                if ((tcp_read_flag = readTCP(neigh_aux->this)) == MSG_FINISH)
                {
                    msg_list = processReadTCP(external, 0);
                    while (msg_list != NULL)
                    {
                        // adicionar aqui o argumento de lixo extra
                        word_count = sscanf(msg_list->message, "%s %s %s\n", command, arg1, arg2);
                        // adicionar aqui a verificação do sscanf e errno e assim
                       
                        // este é o caso em que já tinhamos vizinho externo e recebemos
                        // um novo vizinho que fez connect para nos e enviou o NEW
                        if (!strcmp(command, "NEW") && word_count == 3)
                        {
                            printf("Just received our newly arrived internal's data\n");
                            strncpy(neigh_aux->this->IP, arg1, NI_MAXHOST);
                            strncpy(neigh_aux->this->port, arg2, NI_MAXSERV);
                        }
                    }
                }
            }
        }
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
                    if (external)
                        network_state = TWONODES; // aqui tem de perceber se está TWONODES ou MANYNODES. Pode ser pelo num_nodes 
                    // ou olhando para o backup. Importante que podemos estar twonodes e na verdade haver mais, porque ao entrar
                    // se so estiver la um no ficamos twonodes, e depois ao entrar 1 terceiro no que fica vizinho interno do
                    // nó inicial não vamos saber dele. No entanto, quando tivermos a tabela de encaminhamento feita
                    // podemos ver pelo numero de entradas que temos o numero de nos na rede, visto que vamos ter exatamente uma
                    // entrada por cada no na rede
                    else
                        network_state = ONENODE;
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
                    network_state = NONODES;
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

                        // ao receber a lista, vai selecionar um nó qualquer e ligar-se a ele
                        // de momento, liga-se ao último nó da lista enviada, que (da maneira que a lista é preenchida)
                        // é o primeiro nó da nodes_fucking_list
     
                        num_nodes = 0;
                        // aqui, é preciso escrever código para limpar 
                        // a memória da lista anterior, caso ela exista
                        // na verdade, o correto será apagar a lista
                        // imediatamente após a confirmação de que entrámos no servidor
                        // e portanto não precisamos mais dela
                        nodes_fucking_list = NULL;
                        parseNodeListRecursive(list_msg, &num_nodes, &nodes_fucking_list);
                        external = safeMalloc(sizeof(viz));
                        external->next_av_ix = 0;
                        safeTCPSocket(&(external->fd));
                        connectTCP(nodes_fucking_list->IP, nodes_fucking_list->port, external->fd, 
                                "Error getting address info for external node in JOIN\n", "Error connecting to external node in JOIN\n");

                        tcp_state = waiting_for_backup; // we're outnumbered, need backup
                        network_state = TWONODES; // pelo menos até recebermos a informação do backup, não sabemos se não há apenas 2 nodes
                        // quer dizer, podemos ver pelo num_nodes na verdade
                        
                        // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
                        errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self.IP, self.port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in NEW TCP message creation: %s\n", strerror(errno));
                            exit(-1);
                        }
                        writeTCP(external->fd, strlen(message_buffer), message_buffer);
                        // acabar de preencher a informação do external
                        strncpy(external->IP, nodes_fucking_list->IP, NI_MAXHOST);
                        strncpy(external->port, nodes_fucking_list->port, NI_MAXSERV);
                    }
                    // neste caso a rede está vazia. O nó coloca-se no estado single_node e regista-se diretamente
                    // no servidor de nós
                    else
                    {
                        network_state = ONENODE; // to rule them all
                        // criar string para enviar o registo do nó
                        errcode = snprintf(message_buffer, 150, "REG %u %s %s", self.net, self.IP, self.port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in REG UDP message creation: %s\n", strerror(errno));
                            exit(-1);
                        }
                        sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n");
                        udp_state = waiting_for_regok;
                    }
                }
                else
                    warnOfTrashReceived("WARNING - Received trash through UDP: udp_state waiting_for_list\n", dgram);
            }
        }
        // Ler input do utilizador no terminal
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            user_input = readCommand(&instr_code);
            if (instr_code == JOIN_ID && network_state == NONODES)
            {
                if(sscanf(user_input,"%u %d",&self.net,&self.id) != 2)
                {
                    printf("Error in sscanf JOIN_ID\n");
                    exit(1);
                }
                // MAIS OVERFLOWS
                strcpy(self.IP, argv[1]);
                strcpy(self.port, argv[2]);
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
            else if (instr_code == LEAVE && network_state != NONODES)
            {
                // criar string para enviar o desregisto (?isto é uma palavra) do nó
                errcode = snprintf(message_buffer, 150, "UNREG %u %s %s", self.net, self.IP, self.port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in UNREG UDP message creation: %s\n", strerror(errno));
                    exit(-1);
                }
                sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in UNREG UDP message send\n");
                udp_state = waiting_for_unregok;
                while (int_neighbours)
                {
                    neigh_aux = int_neighbours;
                    int_neighbours = int_neighbours->next;
                    free(neigh_aux);
                    neigh_aux = NULL;
                }
            }
            else if (instr_code == CREATE && network_state != NONODES)
                head = createinsertObject(head,user_input,str_id);
            else if(instr_code == EXIT)
                exit(0);
            free(user_input);
        }
    }	
    return 0;
}
