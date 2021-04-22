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
#include <time.h>

#include "input.h"
#include "udp_parser.h"
#include "errcheck.h"
#include "tcp.h"
#include "routing.h"
#include "search.h"
#include "nodes.h"

#define max(A,B) ((A)>=(B)?(A):(B))
#define SELFFD -1

// enum dos vários estados associados à rede de nós
// NONODES no caso em que não existem nós
// MANYNODES no caso em que a rede tem mais que dois nós
enum {NONODES, ONENODE, MANYNODES} network_state;

void safeExit(char **cache, int N, viz **external, viz **backup, viz **new, tab_entry **first_entry, list_objects **head, list_interest **first_interest, no *self);
void internalLeft(tab_entry **first_entry, int *n_obj, char **cache, viz **external,
                  viz *backup, internals **neigh_aux, internals **neigh_tmp,
                  internals **int_neighbours, list_interest **first_interest, no *self);
void externalLeft(tab_entry **first_entry, int *n_obj, char **cache, viz **external,
        viz *backup,  
        internals **int_neighbours, list_interest **first_interest, no *self);
        
int main(int argc, char *argv[])
{
    int errcode, N;
    char *regIP, *regUDP, *IP, *TCP;
    if(checkEntryArgs(argv, argc, &IP, &TCP, &regIP, &regUDP, &N) == END_EXECUTION) exit(EXIT_FAILURE);

    time_t t;

    srand((unsigned) time(&t));
        
    network_state = NONODES;
    node_list *list_of_nodes, *aux_list_of_nodes, *prev_list_of_nodes;
    fd_set rfds;
    int fd_udp, max_fd, counter, tcp_server_fd, we_are_reg=0, we_register_as_single_node, num_nodes, random_index;
    int external_is_filled, we_used_interest_tmp;
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
    internals *int_neighbours = NULL, *neigh_aux, *neigh_tmp, *neigh_tmp1, *neigh_tmp2;

    tab_entry *first_entry = NULL, *tab_aux;
    list_interest *first_interest = NULL, *interest_aux, *interest_tmp;
    char **cache = createCache(N); 
    int n_obj = 0;
    // estados associados ao select
    enum {not_waiting, waiting_for_list, waiting_for_regok, waiting_for_unregok} udp_state;
    udp_state = not_waiting;
    // lista de mensagens recebidas num readTCP
    messages *msg_list, *msg_aux;
    no self;
    self.id = NULL; self.net = NULL;
    list_objects *head = NULL;
    int flag_no_dest, flag_one_node = 0;
    char net[64], ident[64], name[64], subname[64];
    char *id =  NULL;

    struct sigaction act;
    // Protection against SIGPIPE signals 
    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &act, NULL) == -1)
    {
        fprintf(stderr, "Error in SIGPIPE: %s\n", strerror(errno));
        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
    }

    if ((fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        fprintf(stderr, "Error getting socket for UDP client: %s\n", strerror(errno));
        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
    }
    safeTCPSocket(&tcp_server_fd);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    safeGetAddrInfo(NULL, TCP, &hints, &res, "Error getting address info for TCP server socket\n");
    if (bind(tcp_server_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "Error binding TCP server: %s\n", strerror(errno));
        freeaddrinfo(res);
        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
    }
    freeaddrinfo(res);
    if (listen(tcp_server_fd, 5) == -1)
    {
        fprintf(stderr, "Error putting TCP server to listen: %s\n", strerror(errno));
        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
    }
    helpMenu();

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
        if(counter<=0) exit(1);
        
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

			tab_aux = first_entry;
            while(tab_aux)
            {
                errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n", tab_aux->id_dest);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in ADVERTISE TCP message creation\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);  
                }
                if(writeTCP(new->fd, strlen(message_buffer), message_buffer) == ERROR)
                {
				    freeViz(&new);
                    fprintf(stderr, "error sending ADVERTISE TCP message to new node in network\n");
                    break;
				}
				tab_aux = tab_aux->next;
            }

            // se este processo estava em modo NONODES, ninguém nos deve contactar por TCP
            // porque não estamos ligados a qualquer rede
            if (network_state == NONODES)
            {
                fprintf(stderr, "We're not currently in a new network, but we received a connection through TCP. We closed it immediately.\n");
                freeViz(&new);
            }
            // se este processo estava em modo ONENODE, o primeiro tipo que nos contactar
            // será o nosso vizinho externo
            // Neste caso, apenas aceitamos a conexão e estabelecemos o fd do vizinho externo.
            // Ficamos à espera que ele nos envie o NEW para lhe devolvermos o EXTERN
            else if (network_state == ONENODE)
            {
				printf("We are currently alone in a network. Someone has connected, let's make them our new external neighbour\n");
                external = new;
                new = NULL;
                network_state = MANYNODES;
                external_is_filled = 0;
                flag_one_node = 1;
            } 
            // se este processo estava em modo MANYNODES, alguém que nos contacta
            // será mais um vizinho interno
            else if (network_state == MANYNODES)
            {
                printf("We are currently in a network with 2 or more nodes. Someone has connected, let's make them our new internal neighbour\n");
                // isto é para o caso que está diretamente acima nos comentários, em que não temos os dados do external preenchidos
                // depois enviamos o EXTERN para este interno quando FD_ISSET(external->fd) e recebermos o NEW
                if (external_is_filled)
                {
                    errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                    {
                        fprintf(stderr, "error in EXTERN TCP message creation when we get a new internal neighbour\n");
                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                    }
                    if (writeTCP(new->fd, strlen(message_buffer), message_buffer) == ERROR) 
                    {
                        freeViz(&new);
                        fprintf(stderr, "error sending EXTERN TCP message when we get a new internal neighbour\n");
                    }
                }
                if(new != NULL)
                {
                   addToList(&int_neighbours, new);
                   new = NULL;
                } 
            }
            // não é suposto acontecer alguma vez estas 2 condições
            // isto está aqui apenas por motivos de debug, caso haja alguma falha na máquina de estados
            else
                fprintf(stderr, "Error in finite state machine: network_state CODE 1\n");
            if (new != NULL)
            {
                fprintf(stderr, "Error in finite state machine: network_state CODE 2\n");
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

                    // sempre que recebemos EXTERN devemos atualizar a informação do nosso nó de backup
                    // pode ser quando fizemos connect e enviámos o NEW ou quando o nosso vizinho de backup
                    // fez leave e o nosso vizinho externo se ligou ao seu backup, atualizando o nosso backup
                    if (!strcmp(command, "EXTERN") && word_count == 3)
                    {
                        //waiting_for_backup = 0;
                        printf("Just received the backup data\n");
                        strncpy(backup->IP, arg1, NI_MAXHOST);
                        strncpy(backup->port, arg2, NI_MAXSERV);
                        if (!we_are_reg)
                        {
                            // este é o caso em que nós fizemos connect e enviámos o NEW
                            // vamos armazenar a informação do nosso backup e registar os nossos
                            // dados no servidor de nós

                            // criar string para enviar o registo do nó
                            errcode = snprintf(message_buffer, 150, "REG %s %s %s", self.net, self.IP, self.port);  
                            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                            {
                                fprintf(stderr, "error in REG UDP message creation\n");
                                safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                            }
                            if (sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n") == ERROR)
                            {
                                
                            }
                            udp_state = waiting_for_regok;
                        }
                        we_are_reg = 1;
                        // deviamos colocar aqui a verificação se somos o nosso próprio backup ou nao, isto é two nodes vs many nodes
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
                        external_is_filled = 1;

                        errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in EXTERN TCP message creation when there are only two nodes\n");
                            safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                        }
                        // se o write falhar, fechamos a conexão imediatamente
                        // limpamos a memória de todas as mensagens recebidas do externo
                        // para o qual o write falhou, ignorando-as. Em princípio isto não causará problemas
                        // porque neste caso o externo tinha acabado de se juntar à rede, e não deve haver ainda DATA/NODATA
                        // a vir nas próximas mensagens
                        if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == ERROR)
                        {
                            fprintf(stderr, "Error writing to external neighbour when we receive NEW from it. We shall close the connection now\n");
                            externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);
                            while (msg_list != NULL)
                            {
                                msg_aux = msg_list;
                                msg_list = msg_list->next;
                                free(msg_aux->message);
                                free(msg_aux);
                                msg_aux = NULL;
                            }
                            break;
                        }
                        network_state = MANYNODES; // em princípio este vai sair
                        // nesta situação, o nosso backup vamos ser nós próprios
                        strncpy(backup->IP, IP, NI_MAXHOST); 
                        strncpy(backup->port, TCP, NI_MAXSERV);
                        
                        // enviar para todos os internos os dados do nosso vizinho externo
                        // isto porque caso estivéssemos sozinhos na rede e de repente se ligaram 
                        // vários gajos a nós por connect, podemos ter ficado numa situação em que
                        // aceitámos connect, estávamos MANYNODES mas não tínhamos o nosso vizinho externo preenchido
                        // com o seu IP e porto. Nesse caso, ao recebermos este NEW temos de notificar todos os internos
                        neigh_tmp = NULL;
                        neigh_aux = int_neighbours;
                        while (neigh_aux)
                        {
                            if (writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                            {
                                fprintf(stderr, "Error writing to internal neighbour when we receive NEW from our external. We shall close the connection now\n");
                                internalLeft(&first_entry, &n_obj, cache, &external,
                                             backup, &neigh_aux, &neigh_tmp,
                                             &int_neighbours, &first_interest, &self);
                            }
                            neigh_tmp = neigh_aux;
                            neigh_aux = neigh_aux->next;
                        }
                    }

                    // para quando recebemos uma mensagem ADVERTISE do vizinho externo, reencaminhar esta mensagem para os nossos internos (se houverem)
                    // no final deste processo adicionamos a entrada a nossa tabela de expedição     
                    if (!strcmp(command, "ADVERTISE") && word_count == 2)
                    {
						if(checkTabEntry(first_entry, arg1) == 1)
						{
							if(flag_one_node == 0)
							{
								fprintf(stderr, "There was already a node with the id %s in network %s! We shall close the connection now\n", self.id, self.net);
							    errcode = snprintf(message_buffer, 150, "UNREG %s %s %s", self.net, self.IP, self.port);  
                                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                {
                                    fprintf(stderr, "error in UNREG UDP message creation\n");
                                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                }
                                if (sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", 
                                    "error in UNREG UDP message send\n") == ERROR)
                                    fputs("We encountered an error unregistering from the nodes server. We will proceed execution and ignore it\n", stderr);
                                else
                                    udp_state = waiting_for_unregok;
						    }
						    else
						    fprintf(stderr, "A node with the same id as one in the network tried to join! We shall close the connection now\n");
							
							freeViz(&external);
							if(flag_one_node == 0)
							{
							    freeSelf(&self);
							    FreeTabExp(&first_entry);
							    network_state = NONODES;
						    }
						    else
						    network_state = ONENODE;
							while (msg_list != NULL)
                            {
                                msg_aux = msg_list;
                                msg_list = msg_list->next;
                                free(msg_aux->message);
                                free(msg_aux);
                                msg_aux = NULL;
                            }
                            break;   
						}
						else
						{
						    neigh_tmp = NULL;
                            neigh_aux = int_neighbours;
                            while (neigh_aux != NULL)
                            {
                                if (writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                {
                                    fprintf(stderr, "Error writing to internal neighbour when we receive ADVERTISE from our external. We shall close the connection now\n");
                                    internalLeft(&first_entry, &n_obj, cache, &external,
                                                 backup, &neigh_aux, &neigh_tmp,
                                                 &int_neighbours, &first_interest, &self);
                                }
                                neigh_tmp = neigh_aux;
                                neigh_aux = neigh_aux->next;
                            }
                            first_entry = createinsertTabEntry(first_entry, arg1, external->fd);
					    }
                    }

                    // para quando recebemos uma mensagem WITHDRAW do vizinho externo, reencaminhar esta mensagem para os nossos internos (se houverem)
                    // no final deste processo eliminamos a respetiva entrada da tabela de expedição correspondente ao id do nó que saiu da rede
                    if (!strcmp(command, "WITHDRAW") && word_count == 2)
                    {
						neigh_tmp = NULL;
                        neigh_aux = int_neighbours;
                        while (neigh_aux != NULL)
                        {
                            if (writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                            {
                                fprintf(stderr, "Error writing to internal neighbour when we receive WITHDRAW from our external. We shall close the connection now\n");
                                internalLeft(&first_entry, &n_obj, cache, &external,
                                             backup, &neigh_aux, &neigh_tmp,
                                             &int_neighbours, &first_interest, &self);
                            }
                            neigh_tmp = neigh_aux;
                            neigh_aux = neigh_aux->next;
                        }
                        deleteTabEntryid(&first_entry, arg1);
                        deleteInterestWITHDRAW(&first_interest, arg1);
                        n_obj = deleteCacheid(cache, n_obj, arg1);
                    }

                    //para quando recebemos uma mensagem INTEREST do vizinho externo
                    if (!strcmp(command, "INTEREST") && word_count == 2)
                    {
                        id = NULL;
                        id = getidfromName(arg1, id);

                        // verificar primeiro se o identificador do objeto corresponde ao nosso (se somos o destino da mensagem de interesse)    
                        if(!strcmp(self.id, id))
                        {
                            // caso seja igual verificamos se temos o objeto na nossa lista de objetos
                            if(checkObjectList(head, arg1) == 1)
                            {
                                // se tivermos o objeto na lista de objetos enviamos mensagem DATA de volta ao vizinho externo que nos mandou a mensagem de interesse
                                errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
                                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                {
                                    fprintf(stderr, "error in DATA TCP message creation\n");
                                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                }
                            }
                            else
                            {
                                // se não tivermos o objeto na lista de objetos enviamos mensagem NODATA de volta ao vizinho externo que nos mandou a mensagem de interesse
                                errcode = snprintf(message_buffer, 150, "NODATA %s\n", arg1);  
                                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                {
                                    fprintf(stderr, "error in NODATA TCP message creation\n");
                                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                }
                            }
                            if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == ERROR)
                            {
                                // se o write falhar, fechamos a conexão imediatamente
                                // limpamos a memória de todas as mensagens recebidas do externo
                                // para o qual o write falhou, ignorando-as. Isto poderá causar problemas menores, por exemplo
                                // se a próxima mensagem fosse um DATA/NODATA. No entanto, fizemos assim para simplificar, 
                                // porque caso contrário teríamos de lidar com o facto de que quando lêssemos as próximas mensagens que 
                                // o externo já não se iria referir ao nó que nos enviou as mensagens. Esta lógica aplica-se nos restantes
                                // erros de escrita para o external dentro do FD_ISSET(external->fd)
                                fprintf(stderr, "Error writing to external neighbour when we receive INTEREST from it. We shall close the connection now\n");
                                externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);
                                while (msg_list != NULL)
                                {
                                    msg_aux = msg_list;
                                    msg_list = msg_list->next;
                                    free(msg_aux->message);
                                    free(msg_aux);
                                    msg_aux = NULL;
                                }
                                break;
                            }
                        }
                        else
                        {
                            // caso o identificador do objeto não seja igual ao nosso verificamos se temos o objeto armazenado na cache
                            if(checkCache(cache, arg1, n_obj) == 1)
                            {
                                // se tivermos o objeto na cache enviamos mensagem DATA de volta ao vizinho externo que nos mandou a mensagem de interesse
                                errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
                                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                {
                                    fprintf(stderr, "error in DATA TCP message creation\n");
                                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                }

                                if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == ERROR)
                                {
                                    fprintf(stderr, "Error writing to external neighbour when we receive INTEREST from it. We shall close the connection now\n");
                                    externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);
                                    while (msg_list != NULL)
                                    {
                                        msg_aux = msg_list;
                                        msg_list = msg_list->next;
                                        free(msg_aux->message);
                                        free(msg_aux);
                                        msg_aux = NULL;
                                    }
                                    break;
                                }
                            }
                            else
                            {
                                flag_no_dest = 1;
                                // verifica se ja houve outro pedido do mesmo objeto feito
                                if(checkInterest(first_interest, arg1, external->fd) != 1)
                                    first_interest = addInterest(first_interest, arg1, external->fd);
                                // se não tivermos o objeto na cache, não sendo nós o destino, reencaminhamos a mensagem INTEREST para o próximo nó através da tabela de expedição
                                tab_aux = first_entry;
                                while(tab_aux != NULL)
                                {
                                    if(!strcmp(tab_aux->id_dest, id) && tab_aux->fd_sock != SELFFD)
                                    {
                                        // neste caso, como recebemos a mensagem de interesse vinda do externo, assumindo todas as
                                        // funções de encaminhamento estão OK na rede o tab_aux_fd vai ser o fd de um interno
                                        if(writeTCP(tab_aux->fd_sock, strlen(msg_list->message), msg_list->message) == ERROR)
                                        {
                                            neigh_tmp = NULL;
								            neigh_aux = int_neighbours;
                                            while (neigh_aux->this->fd != tab_aux->fd_sock)
                                            {
                                                neigh_tmp = neigh_aux;
                                                neigh_aux = neigh_aux->next;
                                            }
                                            if (neigh_aux)
                                            {
												fprintf(stderr, "Error forwarding INTEREST to internal neighbour. We shall close the connection now\n");
                                                internalLeft(&first_entry, &n_obj, cache, &external,
                                                        backup, &neigh_aux, &neigh_tmp,
                                                        &int_neighbours, &first_interest, &self);
                                            }
                                            // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                            // não encontrámos um com o fd de tab_aux.
                                            else
                                                fprintf(stderr, "Error! We couldn't find the correct neighbour\n");
                                        }
                                        else
                                        flag_no_dest = 0;
                                    }
                                    tab_aux = tab_aux->next;
                                }
                                if(flag_no_dest == 1)
                                    deleteInterest(&first_interest, arg1, external->fd);
                            }
                        }
                        free(id);
                    }

                    // para quando recebemos uma mensagem DATA do vizinho externo, iteramos a lista de pedidos
                    if (!strcmp(command, "DATA") && word_count == 2)
                    {
                        interest_aux = first_interest;
                        interest_tmp = NULL;
                        we_used_interest_tmp = 0;
                        while(interest_aux != NULL)
                        {
                            // se verificarmos que um pedido para esse objeto tinha fd = -1 quer dizer que somos a origem do pedido
                            if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd == SELFFD)
                            {
                                interest_tmp = interest_aux->next;
                                we_used_interest_tmp = 1;
                                // verificamos se o objeto poderia já estar na cache, se não tiver armazenamos
                                if(checkCache(cache, arg1, n_obj) == 0)
                                {
                                    n_obj++;
                                    n_obj = saveinCache(cache, arg1, n_obj, N);
                                }
                                deleteInterest(&first_interest, arg1, interest_aux->fd);
                            }
                            // se verificarmos que um pedido para esse objeto tinha um fd diferente de -1 significa que foi dum vizinho nosso
                            else if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd != SELFFD)
                            {
                                interest_tmp = interest_aux->next;
                                we_used_interest_tmp = 1;
                                // verificamos se o objeto poderia já estar na cache, se não tiver armazenamos
                                if(checkCache(cache, arg1, n_obj) == 0)
                                {
                                    n_obj++;
                                    n_obj = saveinCache(cache, arg1, n_obj, N);
                                }
                                // reencaminhamos a mensagem DATA para o vizinho que fez esse pedido
                                if(writeTCP(interest_aux->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                {
									neigh_tmp = NULL;
								    neigh_aux = int_neighbours;
                                    while (neigh_aux->this->fd != interest_aux->fd)
                                    {
                                        neigh_tmp = neigh_aux;
                                        neigh_aux = neigh_aux->next;
                                    }
                                    if (neigh_aux)
                                    {
										fprintf(stderr, "Error forwarding DATA to internal neighbour. We shall close the connection now\n");
                                        internalLeft(&first_entry, &n_obj, cache, &external,
                                                        backup, &neigh_aux, &neigh_tmp,
                                                        &int_neighbours, &first_interest, &self);
                                    }
                                    // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                    // não encontrámos um com o fd de interest_aux.
                                    else
                                        fprintf(stderr, "Error! We couldn't find the correct neighbour\n");		
								}
                                deleteInterest(&first_interest, arg1, interest_aux->fd);
                            }
                            if (we_used_interest_tmp)
                            {
                                interest_aux = interest_tmp;
                                interest_tmp = NULL;
                                we_used_interest_tmp = 0;
                            }
                            else
                                interest_aux = interest_aux->next;
                        }
                    }

                    // para quando recebemos uma mensagem NODATA do vizinho externo, iteramos a lista de pedidos
                    if (!strcmp(command, "NODATA") && word_count == 2)
                    {	
                        interest_aux = first_interest;
                        interest_tmp = NULL;
                        we_used_interest_tmp = 0;
                        while(interest_aux != NULL)
                        {
                            // se verificarmos que um pedido para esse objeto tinha fd = -1 quer dizer que somos a origem do pedido
                            if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd == SELFFD)
                            {
                                interest_tmp = interest_aux->next;
                                we_used_interest_tmp = 1;
                                deleteInterest(&first_interest, arg1, interest_aux->fd);
                            }
                            // se verificarmos que um pedido para esse objeto tinha um fd diferente de -1 significa que foi dum vizinho nosso
                            else if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd != SELFFD)
                            {
                                interest_tmp = interest_aux->next;
                                we_used_interest_tmp = 1;
                                // reencaminhamos a mensagem NODATA para o vizinho que fez esse pedido
                                if(writeTCP(interest_aux->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                {
								    neigh_tmp = NULL;
								    neigh_aux = int_neighbours;
                                    while (neigh_aux->this->fd != interest_aux->fd)
                                    {
                                        neigh_tmp = neigh_aux;
                                        neigh_aux = neigh_aux->next;
                                    }
                                    if (neigh_aux)
                                    {
										fprintf(stderr, "Error forwarding NODATA to internal neighbour. We shall close the connection now\n");
                                        internalLeft(&first_entry, &n_obj, cache, &external,
                                                        backup, &neigh_aux, &neigh_tmp,
                                                        &int_neighbours, &first_interest, &self);
                                    }
                                    // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                    // não encontrámos um com o fd de interest_aux.
                                    else
                                        fprintf(stderr, "Error! We couldn't find the correct neighbour\n");   	
								}
                                deleteInterest(&first_interest, arg1, interest_aux->fd);
                            }
                            if (we_used_interest_tmp)
                            {
                                interest_aux = interest_tmp;
                                interest_tmp = NULL;
                                we_used_interest_tmp = 0;
                            }
                            else
                                interest_aux = interest_aux->next;
                        }
                    }

                    msg_aux = msg_list;
                    msg_list = msg_list->next;
                    free(msg_aux->message);
                    free(msg_aux);
                    msg_aux = NULL;
                }
            }
            else if (tcp_read_flag == MSG_CLOSED)
            {
                externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);
            }
        }
        // mudar isto tudo
        neigh_aux = int_neighbours; // neigh_aux irá apontar para o elemento atual da lista
        neigh_tmp = NULL; // neigh_tmp irá apontar para o vizinho interno prévio para motivos de remoção de um elemento da lista
        while (neigh_aux != NULL)
        {
            // de momento de um vizinho interno só recebemos o new
            if (FD_ISSET(neigh_aux->this->fd, &rfds))
            {
                if ((tcp_read_flag = readTCP(neigh_aux->this)) == MSG_FINISH)
                {
                    msg_list = processReadTCP(neigh_aux->this, 0);
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

                        // para quando recebemos uma mensagem ADVERTISE de um interno, reencaminhar esta mensagem para os restantes internos (se houverem), bem como ao nosso externo
                        // no final deste processo adicionamos a entrada a nossa tabela de expedição
                        if (!strcmp(command, "ADVERTISE") && word_count == 2)
                        {
							if(checkTabEntry(first_entry, arg1) == 1)
							{
								fprintf(stderr, "We just received an ADVERTISE from a node with the same id has a node already in the network. We shall close the connection now\n");
                                int_neighbours = int_neighbours->next;
                                if (close(neigh_aux->this->fd))
                                    fprintf(stderr, "error closing file descriptor of internal neighbour: %s\n", strerror(errno));
                                free(neigh_aux->this);
                                free(neigh_aux);
                                neigh_aux = NULL;			
                                while (msg_list != NULL)
                                {
                                    msg_aux = msg_list;
                                    msg_list = msg_list->next;
                                    free(msg_aux->message);
                                    free(msg_aux);
                                    msg_aux = NULL;
                                }
                                break;					
							}
							else
							{
							    neigh_tmp2 = NULL;
                                neigh_tmp1 = int_neighbours;
                                while (neigh_tmp1 != NULL)
                                {
                                    if(neigh_tmp1->this->fd != neigh_aux->this->fd)
                                    {
                                        if(writeTCP(neigh_tmp1->this->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                        {
									        fprintf(stderr, "Error writing to internal neighbour when we receive ADVERTISE from another internal. We shall close the connection now\n");
                                            internalLeft(&first_entry, &n_obj, cache, &external,
                                                     backup, &neigh_tmp1, &neigh_tmp2,
                                                          &int_neighbours, &first_interest, &self);	
									    }
                                    }
                                    neigh_tmp2 = neigh_tmp1;
                                    neigh_tmp1 = neigh_tmp1->next;
                                }
                                if(writeTCP(external->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                {
							        fprintf(stderr, "Error writing to external neighbour when we receive ADVERTISE from an internal. We shall close the connection now\n");
                                    externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
							    }
                                first_entry = createinsertTabEntry(first_entry, arg1, neigh_aux->this->fd);
						    }
                        }

                        // para quando recebemos uma mensagem WITHDRAW de um interno, reencaminhar esta mensagem para os outros internos (se houverem), bem como ao nosso externo
                        // no final deste processo eliminamos a respetiva entrada da tabela de expedição correspondente ao id do nó que saiu da rede
                        if (!strcmp(command, "WITHDRAW") && word_count == 2)
                        {
                            neigh_tmp2 = NULL;
                            neigh_tmp1 = int_neighbours;
                            while (neigh_tmp1 != NULL)
                            {
                                if(neigh_tmp1->this->fd != neigh_aux->this->fd)
                                {
                                    if(writeTCP(neigh_tmp1->this->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                    {
									    fprintf(stderr, "Error writing to internal neighbour when we receive WITHDRAW from another internal. We shall close the connection now\n");
                                        internalLeft(&first_entry, &n_obj, cache, &external,
                                                     backup, &neigh_tmp1, &neigh_tmp2,
                                                          &int_neighbours, &first_interest, &self);	
									}
                                }
                                neigh_tmp2 = neigh_tmp1;
                                neigh_tmp1 = neigh_tmp1->next;
                            }
                            if(writeTCP(external->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                            {
							    fprintf(stderr, "Error writing to external neighbour when we receive WITHDRAW from an internal. We shall close the connection now\n");
                                externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
							}
                            deleteTabEntryid(&first_entry, arg1);
                            deleteInterestWITHDRAW(&first_interest, arg1);
                            n_obj = deleteCacheid(cache, n_obj, arg1);
                        }

                        // para quando recebemos uma mensagem INTEREST dum vizinho interno
                        if (!strcmp(command, "INTEREST") && word_count == 2)
                        {
                            id = NULL;
                            id = getidfromName(arg1, id);

                            // verificar primeiro se o identificador do objeto corresponde ao nosso (se somos o destino da mensagem de interesse)		
                            if(!strcmp(self.id, id))
                            {
                                // caso seja igual verificamos se temos o objeto na nossa lista de objetos
                                if(checkObjectList(head, arg1) == 1)
                                {
                                    // se tivermos o objeto na lista de objetos enviamos mensagem DATA de volta ao vizinho interno que nos mandou a mensagem de interesse
                                    errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
                                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                    {
                                        fprintf(stderr, "error in DATA TCP message creation\n");
                                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                    }
                                }
                                else
                                {
                                    // se não tivermos o objeto na lista de objetos enviamos mensagem NODATA de volta ao vizinho interno que nos mandou a mensagem de interesse
                                    errcode = snprintf(message_buffer, 150, "NODATA %s\n", arg1);  
                                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                    {
                                        fprintf(stderr, "error in NODATA TCP message creation\n");
                                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                    }
                                }
                                if(writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                                {
								    fprintf(stderr, "Error writing to internal neighbour when we receive INTEREST from it. We shall close the connection now\n");
                                    internalLeft(&first_entry, &n_obj, cache, &external,
                                                     backup, &neigh_aux, &neigh_tmp,
                                                          &int_neighbours, &first_interest, &self);	
                                    while (msg_list != NULL)
                                    {
                                        msg_aux = msg_list;
                                        msg_list = msg_list->next;
                                        free(msg_aux->message);
                                        free(msg_aux);
                                        msg_aux = NULL;
                                    }
                                    break;	
								}
                            }
                            else
                            {
                                // caso o identificador do objeto não seja igual ao nosso verificamos se temos o objeto armazenado na cache
                                if(checkCache(cache, arg1, n_obj) == 1)
                                {
                                    // se tivermos o objeto na cache enviamos mensagem DATA de volta ao vizinho interno que nos mandou a mensagem de interesse
                                    errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
                                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                    {
                                        fprintf(stderr, "error in DATA TCP message creation\n");
                                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                    }
                                    if(writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                                    {
								        fprintf(stderr, "Error writing to internal neighbour when we receive INTEREST from it. We shall close the connection now\n");
                                        internalLeft(&first_entry, &n_obj, cache, &external,
                                                     backup, &neigh_aux, &neigh_tmp,
                                                          &int_neighbours, &first_interest, &self);	
                                        while (msg_list != NULL)
                                        {
                                            msg_aux = msg_list;
                                            msg_list = msg_list->next;
                                            free(msg_aux->message);
                                            free(msg_aux);
                                            msg_aux = NULL;
                                        }
                                        break;	
								    }
                                }
                                else
                                {
                                    flag_no_dest = 1;
                                    // verifica se ja houve outro pedido do mesmo objeto feito 
                                    if(checkInterest(first_interest, arg1, neigh_aux->this->fd) != 1)
                                        first_interest = addInterest(first_interest, arg1, neigh_aux->this->fd);
                                    // se não tivermos o objeto na cache, não sendo nós o destino, reencaminhamos a mensagem INTEREST para o próximo nó através da tabela de expedição
                                    tab_aux = first_entry;
                                    while(tab_aux != NULL)
                                    {
                                        if(!strcmp(tab_aux->id_dest, id) && tab_aux->fd_sock != SELFFD)
                                        {
                                            if(writeTCP(tab_aux->fd_sock, strlen(msg_list->message), msg_list->message) == ERROR)
                                            {
												if(external->fd == tab_aux->fd_sock)
												{
												    fprintf(stderr, "Error forwarding INTEREST to external neighbour. We shall close the connection now\n");
                                                    externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
												}
												else
												{
											        neigh_tmp2 = NULL;
								                    neigh_tmp1 = int_neighbours;
                                                    while (neigh_tmp1->this->fd != tab_aux->fd_sock)
                                                    {
                                                        neigh_tmp2 = neigh_tmp1;
                                                        neigh_tmp1 = neigh_tmp1->next;
                                                    }
                                                    if (neigh_tmp1)
                                                    {
													    fprintf(stderr, "Error forwarding INTEREST to other internal neighbour. We shall close the connection now\n");
                                                        internalLeft(&first_entry, &n_obj, cache, &external,
                                                                backup, &neigh_tmp1, &neigh_tmp2,
                                                                &int_neighbours, &first_interest, &self);
                                                    }
                                                    // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                                    // não encontrámos um com o fd de tab_aux.
                                                    else
                                                        fprintf(stderr, "Error! We couldn't find the correct neighbour\n");		
											    }
											}
										    else
                                            flag_no_dest = 0;
                                        }
                                        tab_aux = tab_aux->next;
                                    }
                                    if(flag_no_dest == 1)
                                        deleteInterest(&first_interest, arg1, neigh_aux->this->fd);
                                }
                            }
                            free(id);
                        }

                        // para quando recebemos uma mensagem DATA do vizinho interno, iteramos a lista de pedidos
                        if (!strcmp(command, "DATA") && word_count == 2)
                        {
                            interest_aux = first_interest;
                            interest_tmp = NULL;
                            we_used_interest_tmp = 0;
                            while(interest_aux != NULL)
                            {
                                // se verificarmos que um pedido para esse objeto tinha fd = -1 quer dizer que somos a origem do pedido
                                if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd == SELFFD)
                                {
                                    interest_tmp = interest_aux->next;
                                    we_used_interest_tmp = 1;
                                    // verificamos se o objeto poderia já estar na cache, se não tiver armazenamos
                                    if(checkCache(cache, arg1, n_obj) == 0)
                                    {
                                        n_obj++;
                                        n_obj = saveinCache(cache, arg1, n_obj, N);
                                    }
                                    deleteInterest(&first_interest, arg1, interest_aux->fd);
                                }
                                // se verificarmos que um pedido para esse objeto tinha um fd diferente de -1 significa que foi dum vizinho nosso
                                else if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd != SELFFD)
                                {
                                    interest_tmp = interest_aux->next;
                                    we_used_interest_tmp = 1;
                                    // verificamos se o objeto poderia já estar na cache, se não tiver armazenamos
                                    if(checkCache(cache, arg1, n_obj) == 0)
                                    {
                                        n_obj++;
                                        n_obj = saveinCache(cache, arg1, n_obj, N);
                                    }
                                    // reencaminhamos a mensagem DATA para o vizinho que fez esse pedido
                                    if(writeTCP(interest_aux->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                    {
										if(external->fd == interest_aux->fd)
										{
										    fprintf(stderr, "Error forwarding DATA to external neighbour. We shall close the connection now\n");
                                            externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
									    }
									    else
									    {
									        neigh_tmp2 = NULL;
								            neigh_tmp1 = int_neighbours;
                                            while (neigh_tmp1->this->fd != interest_aux->fd)
                                            {
                                                neigh_tmp2 = neigh_tmp1;
                                                neigh_tmp1 = neigh_tmp1->next;
                                            }
                                            if (neigh_tmp1)
                                            {
												fprintf(stderr, "Error forwarding DATA to other internal neighbour. We shall close the connection now\n");
                                                internalLeft(&first_entry, &n_obj, cache, &external,
                                                            backup, &neigh_tmp1, &neigh_tmp2,
                                                            &int_neighbours, &first_interest, &self);
                                            }
                                            // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                            // não encontrámos um com o fd de interest_aux.
                                            else
                                                fprintf(stderr, "Error! We couldn't find the correct neighbour\n");		
								        }
								    }
                                    deleteInterest(&first_interest, arg1, interest_aux->fd);
                                }
                                if (we_used_interest_tmp)
                                {
                                    interest_aux = interest_tmp;
                                    interest_tmp = NULL;
                                    we_used_interest_tmp = 0;
                                }
                                else
                                    interest_aux = interest_aux->next;
                            }   
                        }

                        // para quando recebemos uma mensagem NODATA do vizinho interno, iteramos a lista de pedidos
                        if (!strcmp(command, "NODATA") && word_count == 2)
                        {
                            interest_aux = first_interest;
                            interest_tmp = NULL;
                            we_used_interest_tmp = 0;
                            while(interest_aux != NULL)
                            {
                                // se verificarmos que um pedido para esse objeto tinha fd = -1 quer dizer que somos a origem do pedido
                                if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd == SELFFD)
                                {
                                    interest_tmp = interest_aux->next;
                                    we_used_interest_tmp = 1;
                                    deleteInterest(&first_interest, arg1, interest_aux->fd);
                                }
                                // se verificarmos que um pedido para esse objeto tinha um fd diferente de -1 significa que foi dum vizinho nosso
                                else if(!strcmp(interest_aux->obj, arg1) && interest_aux->fd != SELFFD)
                                {
                                    interest_tmp = interest_aux->next;
                                    we_used_interest_tmp = 1;
                                    // reencaminhamos a mensagem NODATA para o vizinho que fez esse pedido
                                    if(writeTCP(interest_aux->fd, strlen(msg_list->message), msg_list->message) == ERROR)
                                    {
									    if(external->fd == interest_aux->fd)
										{
										    fprintf(stderr, "Error forwarding NODATA to external neighbour. We shall close the connection now\n");
                                            externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
									    }
									    else
									    {
									        neigh_tmp2 = NULL;
								            neigh_tmp1 = int_neighbours;
                                            while (neigh_tmp1->this->fd != interest_aux->fd)
                                            {
                                                neigh_tmp2 = neigh_tmp1;
                                                neigh_tmp1 = neigh_tmp1->next;
                                            }
                                            if (neigh_tmp1)
                                            {
												fprintf(stderr, "Error forwarding NODATA to other internal neighbour. We shall close the connection now\n");
                                                internalLeft(&first_entry, &n_obj, cache, &external,
                                                            backup, &neigh_tmp1, &neigh_tmp2,
                                                            &int_neighbours, &first_interest, &self);
                                            }
                                            // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                            // não encontrámos um com o fd de interest_aux.
                                            else
                                                fprintf(stderr, "Error! We couldn't find the correct neighbour\n");		
								        }		
								    }
                                    deleteInterest(&first_interest, arg1, interest_aux->fd);
                                }
                                if (we_used_interest_tmp)
                                {
                                    interest_aux = interest_tmp;
                                    interest_tmp = NULL;
                                    we_used_interest_tmp = 0;
                                }
                                else
                                    interest_aux = interest_aux->next;
                            }
                        }

                        msg_aux = msg_list;
                        msg_list = msg_list->next;
                        free(msg_aux->message);
                        free(msg_aux);
                        msg_aux = NULL;
                    }
                    // no caso em que não removemos o vizinho interno da lista
                    // simplesmente avançamos para o vizinho interno seguinte em neigh_aux e colocamos
                    // neigh_tmp a apontar para o vizinho interno atual
                    if(neigh_aux != NULL)
                    {
                        neigh_tmp = neigh_aux;
                        neigh_aux = neigh_aux->next;
                    }
                }
                // vizinho interno fez leave e fechou as conexões
                else if (tcp_read_flag == MSG_CLOSED)
                {
                    internalLeft(&first_entry, &n_obj, cache, &external,
                            backup, &neigh_aux, &neigh_tmp,
                            &int_neighbours, &first_interest, &self);
                    // adicionar as verificações dos outros tcp_read_flag possíveis
                }
            }
            // no caso em que não está set, também temos de iterar pela lista
            else
            {
                neigh_tmp = neigh_aux;
                neigh_aux = neigh_aux->next;
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
                        network_state = MANYNODES; 
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
                we_register_as_single_node = 1;
                // fazer função que verifica se temos uma lista de nós
                list_msg = isNodesList(dgram, self.net, &flag);
                // neste caso não recebemos
                if (flag)
                {
                    first_entry = createinsertTabEntry(first_entry, self.id, SELFFD);
                    printf("We received the list of nodes from the server\n");
                    // assign a random node 
                    if (list_msg)
                    {
                        // ao receber a lista, vai selecionar um nó qualquer e ligar-se a ele
                        // de momento, liga-se ao último nó da lista enviada, que (da maneira que a lista é preenchida)
                        // é o primeiro nó da list_of_nodes
                        // aqui, é preciso escrever código para limpar 
                        // a memória da lista anterior, caso ela exista
                        // na verdade, o correto será apagar a lista
                        // imediatamente após a confirmação de que entrámos no servidor
                        // e portanto não precisamos mais dela
                        list_of_nodes = NULL;
                        num_nodes = 0;
                        parseNodeListRecursive(list_msg, &list_of_nodes, &num_nodes);
                        external = safeMalloc(sizeof(viz));
                        external->next_av_ix = 0;
                        // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
                        errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self.IP, self.port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in NEW TCP message creation\n");
                            safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                        }
                        errcode = ERROR;
                        
                        // neste caso havia nós na lista
                        if (list_of_nodes)
                        {
                            // escolher um nó da lista pseudo-aleatório 
                            aux_list_of_nodes = list_of_nodes;
                            random_index = rand() % num_nodes;
                            prev_list_of_nodes = NULL;
                            for (int i = 0; i < random_index; i++)
                            {
                                prev_list_of_nodes = aux_list_of_nodes;
                                aux_list_of_nodes = aux_list_of_nodes->next;
                            }
                            external->fd = socket(AF_INET, SOCK_STREAM, 0);
                            if (external->fd == -1){
                                fputs("Error making a TCP socket for external neighbour after receiving nodes list!\n", stderr);
                                fprintf(stderr, "error: %s\n", strerror(errno));
                                safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                            }
                            errcode = connectTCP(aux_list_of_nodes->IP, aux_list_of_nodes->port, external->fd, 
                                    "Error getting address info for external node in JOIN\n", "Error connecting to external node in JOIN\n");
                            // neste caso o errcode é NO_ERROR se o connect funcionou
                            // tentamos enviar as 2 mensagens por TCP que temos de enviar
                            // se alguma delas falhar no envio, consideramos que não nos conseguimos ligar e tentamo-nos ligar ao seguinte nó da lista recebida
                            if (errcode == NO_ERROR)
                            {
                                if (writeTCP(external->fd, strlen(message_buffer), message_buffer))
                                {
                                    errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n",self.id);  
                                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                    {
                                        fprintf(stderr, "error in ADVERTISE TCP message creation\n");
                                        errcode = ERROR;
                                    }
                                    else
                                        errcode = NO_ERROR;
                                    if (!writeTCP(external->fd, strlen(message_buffer), message_buffer))
                                        errcode = ERROR;
                                }
                                else
                                    errcode = ERROR;
                            }
                            // neste caso houve um erro ao tentar ligar ao nó escolhido aleatoriamente
                            if (errcode == ERROR)
                            {
                                if (close(external->fd) == -1)
                                    fprintf(stderr, "Error closing file descriptor for unavailable randomly chosen external neighbour: %s\n", strerror(errno));
                                // retiramos o nó da lista, para não o tentarmos de novo
                                
                                // caso o nó fosse o primeiro, simplesmente começamos a iterar a partir do segundo
                                if (!prev_list_of_nodes)
                                    aux_list_of_nodes = list_of_nodes->next;
                                // caso contrário, removemos mesmo e limpamos a memória
                                else
                                {
                                    prev_list_of_nodes->next = aux_list_of_nodes->next;
                                    free(aux_list_of_nodes);
                                    aux_list_of_nodes = list_of_nodes; // começamos a tentar do início da lista criada, que é o fim da enviada
                                }
                                while (aux_list_of_nodes)
                                {
                                    external->fd = socket(AF_INET, SOCK_STREAM, 0);
                                    if (external->fd == -1){
                                        fputs("Error making a TCP socket for external neighbour after receiving nodes list!\n", stderr);
                                        fprintf(stderr, "error: %s\n", strerror(errno));
                                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                    }
                                    errcode = connectTCP(aux_list_of_nodes->IP, aux_list_of_nodes->port, external->fd, 
                                            "Error getting address info for external node in JOIN\n", "Error connecting to external node in JOIN\n");
                                    // neste caso o errcode é NO_ERROR se o connect funcionou
                                    // tentamos enviar as 2 mensagens por TCP que temos de enviar
                                    // se alguma delas falhar no envio, consideramos que não nos conseguimos ligar e tentamo-nos ligar ao seguinte nó da lista recebida
                                    if (errcode == NO_ERROR)
                                    {
                                        if (writeTCP(external->fd, strlen(message_buffer), message_buffer))
                                        {
                                            errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n",self.id);  
                                            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                            {
                                                fprintf(stderr, "error in ADVERTISE TCP message creation\n");
                                                errcode = ERROR;
                                            }
                                            else
                                                errcode = NO_ERROR;
                                            if (!writeTCP(external->fd, strlen(message_buffer), message_buffer))
                                                errcode = ERROR;
                                        }
                                        else
                                            errcode = ERROR;
                                    }
                                    // neste caso o errcode será NO_ERROR se tanto o connect como ambos os writes funcionaram
                                    if (errcode == NO_ERROR)
                                        break;
                                    // segundo a manpage do connect, se falhar o connect temos de dar close da socket
                                    // caso falhe algum dos write também vamos fechar a socket
                                    else if (close(external->fd) == -1)
                                        fprintf(stderr, "Error closing file descriptor for unavailable external neighbour: %s\n", strerror(errno));
                                    aux_list_of_nodes = aux_list_of_nodes->next;
                                }
                            }
                        }
                        // neste caso não nos conseguimos ligar a nenhum dos nós listados no servidor de nomes
                        // vamos entrar na rede como nó único, assumindo que os outros falharam de algum modo
                        // ou estão no processo
                        // de enviar o UNREG
                        if (errcode == ERROR)
                        {
                            free(external);
                            external = NULL;
                            network_state = ONENODE; 
                        }  
                        // caso em que nos conseguimos ligar a um dos nós listados no servidor de nomes
                        else
                        {
                            //waiting_for_backup = 1; // we're outnumbered, need backup
                            network_state = MANYNODES;                     
                            // aqui devíamos colocar um estado novo em que ficamos à espera do extern

                            // acabar de preencher a informação do external
                            strncpy(external->IP, aux_list_of_nodes->IP, NI_MAXHOST);
                            strncpy(external->port, aux_list_of_nodes->port, NI_MAXSERV);
                            external_is_filled = 1;
                            we_register_as_single_node = 0;
                        }
                        // clear list
                        freeNodeList(&list_of_nodes);
                    }
                    // neste caso a rede está vazia ou não nos conseguimos ligar a nenhum dos listados. O nó coloca-se no estado ONENODE regista-se diretamente
                    // no servidor de nós
                    if (we_register_as_single_node)
                    {
                        network_state = ONENODE; // to rule them all
                        // criar string para enviar o registo do nó
                        errcode = snprintf(message_buffer, 150, "REG %s %s %s", self.net, self.IP, self.port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in REG UDP message creation\n");
                            safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                        }
                        sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n");
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
                memset(net, 0, 64);
                memset(ident, 0, 64);
                self.net = NULL;
                self.id = NULL;

                if(sscanf(user_input,"%s %s", net, ident) != 2)
                {
                    printf("Error in sscanf JOIN_ID\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }

                self.net = safeMalloc(strlen(net)+1);
                strcpy(self.net, net);
                self.id = safeMalloc(strlen(ident)+1);
                strcpy(self.id, ident);

                strncpy(self.IP, IP, NI_MAXHOST);
                strncpy(self.port, TCP, NI_MAXSERV);

                // criar string para enviar o pedido de nós
                errcode = snprintf(message_buffer, 150, "NODES %s", self.net);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 100)
                {
                    fprintf(stderr, "error in NODES UDP message creation\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }

                // não limpamos a memória do self.IP e do self.port porque não há necessidade
                if (sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", "error in JOIN UDP message send\n") == ERROR)
                    freeSelf(&self);
                else
                {
                    udp_state = waiting_for_list;
                    external_is_filled = 0;
                }
            }
            else if (instr_code == JOIN_LINK && network_state == NONODES)
            {
                memset(net, 0, 64);
                memset(ident, 0, 64);
                self.net = NULL;
                self.id = NULL;
                // esta condição em princípio nunca será necessária
                // visto que quando iniciamos o programa o external é colocado a NULL
                // e quando fazemos LEAVE também. Portanto temos sempre de alocar memória
                if (!external)
                    external = safeMalloc(sizeof(viz));
                external_is_filled = 1;
                // aqui crashamos o programa, porque sendo os 4 argumentos strings
                // se falhar o sscanf provavelmente passa-se algo de muito mal.
                if(sscanf(user_input,"%s %s %s %s", net, ident, external->IP, external->port ) != 4)
                {
                    printf("Error in sscanf JOIN_LINK\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }

                self.net = safeMalloc(strlen(net)+1);
                strcpy(self.net, net);
                self.id = safeMalloc(strlen(ident)+1);
                strcpy(self.id, ident);

                strncpy(self.IP, IP, NI_MAXHOST);
                strncpy(self.port, TCP, NI_MAXSERV);

                external->next_av_ix = 0;

                if ((external->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
                    fprintf(stderr, "Error making a TCP socket for external neighbour in JOIN_LINK: %s\n", strerror(errno));
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }
                if(connectTCP(external->IP, external->port, external->fd, 
                            "Error getting address info for external node in JOIN_LINK\n", "Error connecting to external node in JOIN_LINK\n") == ERROR)
                {
                    freeViz(&external);
                    freeSelf(&self);
                    fputs("We couldn't connect to the selected external neighbour\n", stderr);
                }
                else
                {
                    //waiting_for_backup = 1; // we're outnumbered, need backup
                    // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
                    errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self.IP, self.port);  
                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                    {
                        fprintf(stderr, "error in NEW TCP message creation\n");
                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                    }
                    if (writeTCP(external->fd, strlen(message_buffer), message_buffer)== ERROR)
                    {
                        freeViz(&external);
                        freeSelf(&self);
                        fputs("We couldn't write to the selected external neighbour\n",stderr);
                    }
                    else
                    {
                        errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n",self.id);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            fprintf(stderr, "error in ADVERTISE TCP message creation\n");
                            safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                        }
                        if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == ERROR)
                        {
                            freeViz(&external);
                            freeSelf(&self);
                            fputs("We couldn't write to the selected external neighbour\n", stderr);
                        }
                        // neste caso não houve erro nenhum
                        else 
                        {
                            first_entry = createinsertTabEntry(first_entry, self.id, SELFFD);
                            network_state = MANYNODES; // pelo menos até recebermos a informação do backup, não sabemos se não há apenas 2 nodes
                        }
                    }
                }
            }
            

            // versão extra do JOIN - permite criar a rede e executar a funcionalidade sem servidor UDP ativo
            // simplesmente se coloca 
            else if(instr_code == JOIN_SERVER_DOWN && network_state == NONODES)
            {
                network_state = ONENODE;
                we_are_reg = 1;

                if(sscanf(user_input,"%s %s",self.net,self.id) != 2)
                {
                    printf("Error in sscanf JOIN_LINK\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }
                strncpy(self.IP, argv[1], NI_MAXHOST);
                strncpy(self.port, argv[2], NI_MAXSERV);
            }
            else if (instr_code == LEAVE && network_state != NONODES)
            {
				flag_one_node = 0;
                we_are_reg = 0;
                // criar string para enviar o desregisto (?isto é uma palavra) do nó
                errcode = snprintf(message_buffer, 150, "UNREG %s %s %s", self.net, self.IP, self.port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in UNREG UDP message creation\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }
                if (sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", 
                            "error in UNREG UDP message send\n") == ERROR)
                    fputs("We encountered an error unregistering from the nodes server. We will proceed execution and ignore it\n", stderr);
                else
                    udp_state = waiting_for_unregok;

                // após tirar o registo no servidor de nós, devemos terminar todas as conexões TCP ativas 

                // terminar a sessão TCP com o vizinho externo
                freeViz(&external);
                // terminar todas as conexões com vizinhos internos
                // e limpar a memória da lista
                freeIntNeighbours(&int_neighbours);
                freeSelf(&self);
                FreeTabExp(&first_entry);
                FreeObjectList(&head);
                clearCache(cache,n_obj);
                FreeInterestList(&first_interest);
                n_obj = 0;
                // indicar que não estamos ligados a qualquer rede
                network_state = NONODES;
            }
            else if (instr_code == CREATE && network_state != NONODES)
            {
                memset(subname, 0, 64);
                if(sscanf(user_input,"%s", subname) != 1)
                {
                    printf("Error in sscanf CREATE\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }
                head = createinsertObject(head,subname,self.id);
                printObjectList(head);
            }
            else if (instr_code == GET && network_state != NONODES)
            {
                memset(name, 0, 64);
                if(sscanf(user_input,"%s", name) != 1)
                {
                    printf("Error in sscanf GET\n");
                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                }
                id = NULL;
                id = getidfromName(name, id);    

                if(checkCache(cache, name, n_obj) == 1)
                {
                    printf("Object %s was already in our own cache!\n", name);
                }
                else
                {
                    if(checkTabEntry(first_entry,id) == 1)
                    {
                        tab_aux = first_entry;
                        while(tab_aux != NULL)
                        {
                            if(!strcmp(tab_aux->id_dest, id) && tab_aux->fd_sock == SELFFD)
                            {
                                if(checkObjectList(head, name) == 1)
                                {
                                    printf("Object %s was already in our own list of objects!\n", name);
                                }
                                else
                                {
                                    printf("Object %s isn't in our own list of objects!\n", name);
                                }
                            }
                            if(!strcmp(tab_aux->id_dest, id) && tab_aux->fd_sock != SELFFD)
                            {
                                errcode = snprintf(message_buffer, 150, "INTEREST %s\n", name);  
                                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                                {
                                    fprintf(stderr, "error in INTEREST TCP message creation\n");
                                    safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                                }
                                if(writeTCP(tab_aux->fd_sock, strlen(message_buffer), message_buffer) == ERROR)
                                {
								    if(external->fd == tab_aux->fd_sock)
									{
						                fprintf(stderr, "Error sending INTEREST to external neighbour. We shall close the connection now\n");
                                        externalLeft(&first_entry, &n_obj, cache, &external, backup, &int_neighbours, &first_interest, &self);	
									}
									else
									{
										neigh_tmp2 = NULL;
								        neigh_tmp1 = int_neighbours;
                                        while (neigh_tmp1->this->fd != tab_aux->fd_sock)
                                        {
                                            neigh_tmp2 = neigh_tmp1;
                                            neigh_tmp1 = neigh_tmp1->next;
                                        }
                                        if (neigh_tmp1)
                                        {
										    fprintf(stderr, "Error sending INTEREST to internal neighbour. We shall close the connection now\n");
                                            internalLeft(&first_entry, &n_obj, cache, &external,
                                                                backup, &neigh_tmp1, &neigh_tmp2,
                                                                &int_neighbours, &first_interest, &self);
                                        }
                                        // este else não é suposto acontecer nunca. significa que iterámos por todos os vizinhos internos
                                        // não encontrámos um com o fd de tab_aux.
                                        else
                                            fprintf(stderr, "Error! We couldn't find the correct neighbour\n");		
								    }	
								}
								else
								{
                                    if(checkInterest(first_interest, name, SELFFD) != 1)
                                        first_interest = addInterest(first_interest, name, SELFFD);
                                }
                            }
                            tab_aux = tab_aux->next;
                        }
                    }
                    else
                    {
                        printf("Node with the identifier %s does not exist in the network %s!\n", id, self.net);
                    }
                }
                free(id);
            }
            else if(instr_code == EXIT)
            {
                if (network_state != NONODES)
                {
                    we_are_reg = 0;
                    // criar string para enviar o desregisto (?isto é uma palavra) do nó
                    errcode = snprintf(message_buffer, 150, "UNREG %s %s %s", self.net, self.IP, self.port);  
                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                    {
                        fprintf(stderr, "error in UNREG UDP message creation\n");
                        safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
                    }
                    if (sendUDP(fd_udp, regIP, regUDP, message_buffer, "Error getting address information for UDP server socket\n", 
                                "error in UNREG UDP message send\n") == ERROR)
                        fputs("We encountered an error unregistering from the nodes server. We will proceed execution and ignore it\n", stderr);
                    else
                        udp_state = waiting_for_unregok;

                    // após tirar o registo no servidor de nós, devemos terminar todas as conexões TCP ativas 
                    // terminar a sessão TCP com o vizinho externo
                    freeViz(&external);
                    // terminar todas as conexões com vizinhos internos
                    // e limpar a memória da lista
                    freeIntNeighbours(&int_neighbours);
                    freeSelf(&self);
                    FreeTabExp(&first_entry);
                    FreeObjectList(&head);
                    clearCache(cache,n_obj);
                    FreeInterestList(&first_interest);
                    n_obj = 0;
                    // indicar que não estamos ligados a qualquer rede
                    network_state = NONODES;
                }
                safeExit(cache, N, &external, &backup, &new, &first_entry, &head, &first_interest, &self);
            }
            else if(instr_code == ST)
            {
                if (network_state == NONODES)
                {
                    printf("State is NONODES\n");
                    printf("We're not connected to any network\n");
                }
                else if (network_state == ONENODE)
                {
                    printf("State is ONENODE\n");
                    printf("We're alone in network %s!\n", self.net);         
                }
                else if (network_state == MANYNODES)
                {
                    printf("State is MANYNODES\n");
                    neigh_aux = int_neighbours;
                    while (neigh_aux)
                    {
                        printf("Internal neighbour's contact: %s:%s\n", neigh_aux->this->IP, neigh_aux->this->port);
                        neigh_aux = neigh_aux->next;
                    }
                    printf("External neighbour's contact: %s:%s\n", external->IP, external->port);
                    printf("Backup's contact: %s:%s\n", backup->IP, backup->port);
                }
                else
                    printf("Error in FSM: show topology\n");
            }
            else if(instr_code == SR && network_state != NONODES)
            {
                printTabExp(first_entry);
            }
            else if(instr_code == SC && network_state != NONODES)
            {
                printCache(cache,n_obj, N);		
            }
            else if(network_state == NONODES && (instr_code == SR || instr_code == SC || instr_code == GET || instr_code == CREATE || instr_code == LEAVE))
            {
                printf("We're not connected to any network\n");
            }
            else if(network_state != NONODES && (instr_code == JOIN_ID || instr_code == JOIN_LINK))
            {
                printf("We're already connected to network %s!\n",self.net);
            }
            free(user_input);
        }
    }	
    return 0;
}

void safeExit(char **cache, int N, viz **external, viz **backup, viz **new,
        tab_entry **first_entry, list_objects **head, list_interest **first_interest,
        no *self)
{
    freeCache(cache, N);
    freeViz(external);
    free(*backup);
    *backup = NULL;
    freeViz(new);
    FreeTabExp(first_entry);
    FreeObjectList(head);
    FreeInterestList(first_interest);
    freeSelf(self);
    exit(1);
}

void externalLeft(tab_entry **first_entry, int *n_obj, char **cache, viz **external,
        viz *backup, internals **int_neighbours, list_interest **first_interest, no *self)
{
    char message_buffer[150];
    tab_entry *tab_aux = *first_entry;
    tab_entry *tab_tmp = NULL;
    int we_used_tab_tmp = 0, errcode, we_used_interest_tmp = 0;
    internals *neigh_aux, *neigh_tmp;
    list_interest *interest_aux, *interest_tmp;
    while(tab_aux != NULL)
    {
        if(tab_aux->fd_sock == (*external)->fd)
        {
            tab_tmp = tab_aux->next;
            we_used_tab_tmp = 1;
            errcode = snprintf(message_buffer, 150, "WITHDRAW %s\n", tab_aux->id_dest);
            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
            {
                fprintf(stderr, "error in WITHDRAW TCP message creation\n");
                exit(-1);
            }
            neigh_tmp = NULL;
            neigh_aux = *int_neighbours;
            while (neigh_aux != NULL)
            {
                if(writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                {
                    fprintf(stderr, "Error writing WITHDRAW to internal neighbour. We shall close the connection now\n");
                    internalLeft(first_entry, n_obj, cache, external, backup, &neigh_aux, &neigh_tmp, int_neighbours, first_interest, self);
			    }
                neigh_tmp = neigh_aux;
                neigh_aux = neigh_aux->next;
            }
            deleteInterestWITHDRAW(first_interest, tab_aux->id_dest);
            *n_obj = deleteCacheid(cache, *n_obj, tab_aux->id_dest);
            deleteTabEntryfd(first_entry, tab_aux->fd_sock);
        }
        if (we_used_tab_tmp)
        {    
            tab_aux = tab_tmp;
            tab_tmp = NULL;
            we_used_tab_tmp = 0;
        }
        else
            tab_aux = tab_aux->next;
    }

    interest_aux = *first_interest;
    interest_tmp = NULL;
    we_used_interest_tmp = 0;
    while(interest_aux != NULL)
    {
        if(interest_aux->fd == (*external)->fd)
        {
            interest_tmp = interest_aux->next;
            we_used_interest_tmp = 1;
            deleteInterestfd(first_interest, interest_aux->fd);
        }
        if (we_used_interest_tmp)
        {
            interest_aux = interest_tmp;
            interest_tmp = NULL;
            we_used_interest_tmp = 0;
        }
        else
            interest_aux = interest_aux->next;
    }

    if ((errcode = close((*external)->fd)))
        fprintf(stderr, "error closing external fd when external closes connection or there is an error writing to it: %s\n", strerror(errno));

    // neste caso nós somos o nosso próprio vizinho de backup
    if (!strcmp(self->IP, backup->IP) && !strcmp(self->port, backup->port))
    {
        printf("We were abandoned by our external neighbour and our backup neighbour was us\n");
        free(*external);
        // neste caso vamos promover um dos vizinhos internos a vizinho externo
        // o vizinho que vai ser promovido vai ser o do topo da lista
        if (*int_neighbours)
        {
            *external = (*int_neighbours)->this;
            neigh_aux = *int_neighbours;
            *int_neighbours = (*int_neighbours)->next;     
            free(neigh_aux);
            // enviar um EXTERN com os dados do vizinho interno promovido a vizinho externo
            // para o notificar  
            errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", (*external)->IP, (*external)->port);  
            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
            {
                fprintf(stderr, "error in EXTERN TCP message creation\n");
                exit(-1);
            }
            if(writeTCP((*external)->fd, strlen(message_buffer), message_buffer) == ERROR)
            {
			    fprintf(stderr, "Error writing EXTERN to external (old internal). We shall close the connection now\n");
                externalLeft(first_entry, n_obj, cache, external, backup, int_neighbours, first_interest, self);	
			}
            // para todos os vizinhos internos que não foram promovidos a vizinhos externos, notificá-los também
            // que têm um novo vizinho externo
            neigh_tmp = NULL;
            neigh_aux = *int_neighbours;
            while (neigh_aux != NULL)
            {
                if(writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                {
                    fprintf(stderr, "Error writing EXTERN to internal neighbour. We shall close the connection now\n");
                    internalLeft(first_entry, n_obj, cache, external, backup, &neigh_aux, &neigh_tmp, int_neighbours, first_interest, self);
			    }
                neigh_tmp = neigh_aux;
                neigh_aux = neigh_aux->next;
            }
        }
        // caso fôssemos o nosso próprio backup e não tivéssemos vizinhos 
        // internos, então vamos sozinhos numa rede
        else
        {
            printf("We had no internal neighbours\n");
            network_state = ONENODE;
            *external = NULL;
        }
    }

    // neste caso temos um backup, tornamos esse backup o nosso externo
    else
    {
        // atualizar a informação do vizinho externo
        strncpy((*external)->IP, backup->IP, NI_MAXHOST);
        strncpy((*external)->port, backup->port, NI_MAXSERV);
        (*external)->next_av_ix = 0;
        safeTCPSocket(&((*external)->fd));
        // ligar ao vizinho de recuperação (futuro vizinho externo)
        connectTCP((*external)->IP,(*external)->port, (*external)->fd, 
                "Error getting address info for external node when previous external closes connection\n", 
                "Error connecting to external node when previous external closes connection\n");

        // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
        errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self->IP, self->port);  
        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
        {
            fprintf(stderr, "error in NEW TCP message creation when previous external closes connection\n");
            exit(-1);
        }
        if(writeTCP((*external)->fd, strlen(message_buffer), message_buffer) == ERROR)
        {
		    fprintf(stderr, "Error writing NEW to external. We shall close the connection now\n");
            externalLeft(first_entry, n_obj, cache, external, backup, int_neighbours, first_interest, self);	
		}
        //waiting_for_backup = 1; // we're outnumbered, need backup
        //se tivermos internos que não sabem ainda que o nosso externo (o seu backup) mudou, notificá-los através da mensagem EXTERN
        if(*int_neighbours)
        {
            errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", (*external)->IP, (*external)->port);  
            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
            {
                fprintf(stderr, "error in EXTERN TCP message creation\n");
                exit(-1);
            }
            neigh_tmp = NULL;
            neigh_aux = *int_neighbours;
            while (neigh_aux != NULL)
            {
                if(writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                {
                    fprintf(stderr, "Error writing EXTERN to internal neighbour. We shall close the connection now\n");
                    internalLeft(first_entry, n_obj, cache, external, backup, &neigh_aux, &neigh_tmp, int_neighbours, first_interest, self);
			    }
                neigh_tmp = neigh_aux;
                neigh_aux = neigh_aux->next;
            }
        }
        // possivelmente deixar alguma informação na estrutura do backup que está desatualizado
        // mas como estamos waiting_for_backup, podemos deduzir isso daí
        
        tab_aux = *first_entry;
        while(tab_aux)
        {
            errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n", tab_aux->id_dest);  
            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
            {
                fprintf(stderr, "error in ADVERTISE TCP message creation\n");
                exit(-1);  
            }
            if(writeTCP((*external)->fd, strlen(message_buffer), message_buffer) == ERROR)
            {
                fprintf(stderr, "error sending ADVERTISE TCP message to external neighbour. We shall close the connection now\n");
                externalLeft(first_entry, n_obj, cache, external, backup, int_neighbours, first_interest, self);
                break;
	        }
		    tab_aux = tab_aux->next;
        }
    }
}

void internalLeft(tab_entry **first_entry, int *n_obj, char **cache, viz **external,
        viz *backup, internals **neigh_aux, internals **neigh_tmp,
        internals **int_neighbours, list_interest **first_interest, no *self)
{
    char message_buffer[150];
    tab_entry *tab_aux = *first_entry;
    tab_entry *tab_tmp = NULL;
    int we_used_tab_tmp = 0, errcode, we_used_interest_tmp = 0;
    internals *neigh_tmp1, *neigh_tmp2;
    list_interest *interest_aux, *interest_tmp;
    while(tab_aux != NULL)
    {
        if(tab_aux->fd_sock == (*neigh_aux)->this->fd)
        {
            tab_tmp = tab_aux->next;
            we_used_tab_tmp = 1;
            errcode = snprintf(message_buffer, 150, "WITHDRAW %s\n", tab_aux->id_dest);  
            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
            {
                fprintf(stderr, "error in WITHDRAW TCP message creation\n");
                exit(-1);
            }
            neigh_tmp2 = NULL;
            neigh_tmp1 = *int_neighbours;
            while (neigh_tmp1 != NULL)
            {
                if(neigh_tmp1->this->fd != (*neigh_aux)->this->fd) 
                {
                    if(writeTCP(neigh_tmp1->this->fd, strlen(message_buffer), message_buffer) == ERROR)
                    {
                        fprintf(stderr, "Error writing WITHDRAW to different internal neighbour. We shall close the connection now\n");
                        internalLeft(first_entry, n_obj, cache, external, backup, &neigh_tmp1, &neigh_tmp2, int_neighbours, first_interest, self);
				    }
                }
                neigh_tmp2 = neigh_tmp1;
                neigh_tmp1 = neigh_tmp1->next;
            }
            if(writeTCP((*external)->fd, strlen(message_buffer), message_buffer) == ERROR)
            {
		        fprintf(stderr, "Error writing WITHDRAW to external neighbour. We shall close the connection now\n");
                externalLeft(first_entry, n_obj, cache, external, backup, int_neighbours, first_interest, self);	
		    }
            deleteInterestWITHDRAW(first_interest, tab_aux->id_dest);
            *n_obj = deleteCacheid(cache, *n_obj, tab_aux->id_dest);
            deleteTabEntryfd(first_entry, tab_aux->fd_sock);
        }
        if (we_used_tab_tmp)
        {
            tab_aux = tab_tmp;
            tab_tmp = NULL;
            we_used_tab_tmp = 0;
        }
        else
            tab_aux = tab_aux->next;
    }

    interest_aux = *first_interest;
    interest_tmp = NULL;
    we_used_interest_tmp = 0;
    while(interest_aux != NULL)
    {
        if(interest_aux->fd == (*neigh_aux)->this->fd)
        {
            interest_tmp = interest_aux->next;
            we_used_interest_tmp = 1;
            deleteInterestfd(first_interest, interest_aux->fd);
        }
        if (we_used_interest_tmp)
        {
            interest_aux = interest_tmp;
            interest_tmp = NULL;
            we_used_interest_tmp = 0;
        }
        else
            interest_aux = interest_aux->next;
    }

    close((*neigh_aux)->this->fd); // fechar o fd do tipo que fez close do outro lado
    // remover o vizinho interno da lista

    // neste caso, o nó que vamos remover da lista
    // era o topo da lista
    if (*neigh_tmp == NULL)
    {
        *int_neighbours = (*int_neighbours)->next;
        if (!*int_neighbours)
            printf("We no longer have internal neighbours\n");
        // limpar a memória do vizinho interno removido        
        free((*neigh_aux)->this);
        free(*neigh_aux);
        *neigh_aux = NULL;

        // o nó prévio fica a NULL, não é preciso alterar nada

        // o nó por onde vamos iterar a seguir é o novo topo da lista
        *neigh_aux = *int_neighbours;
    }
    // neste caso o nó que vamos remover da lista
    // estava ou no meio ou na cauda
    else
    {
        // ligar o nosso predecessor (neigh_tmp) ao nosso sucessor (neigh_aux->next)
        (*neigh_tmp)->next = (*neigh_aux)->next;
        // limpar a memória do vizinho interno removido        
        free((*neigh_aux)->this);
        free(*neigh_aux);
        // o nó por onde vamos iterar a seguir é o sucessor do nó que acabámos de remover (que está guardado em neigh_tmp->next)
        *neigh_aux = (*neigh_tmp)->next;
    }
    // adicionar as verificações dos outros tcp_read_flag possíveis
}
