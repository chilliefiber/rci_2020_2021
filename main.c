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
#define SELFFD -1

typedef struct tab_entry{
    int id_dest;
    int fd_sock;
    struct tab_entry *next;
}tab_entry;

typedef struct cache_objects{
    char *obj;
}cache_objects;

typedef struct no{
    unsigned int net; //identificador da rede
    int id;    //identificador do nó
    char IP[NI_MAXHOST]; //endereço IP do nó
    char port[NI_MAXSERV]; //Porto TCP do nó
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

tab_entry *createinsertTabEntry(tab_entry *first_entry, int id_dst, int fd)
{
    tab_entry *tmp = first_entry;
    tab_entry *new_entry = safeMalloc(sizeof(tab_entry));
	
    new_entry->id_dest = id_dst;
    new_entry->fd_sock = fd;
	
    if(first_entry == NULL)
    {
	first_entry = new_entry;
	new_entry->next = NULL;
    }
    else
    {
	while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = new_entry;
        new_entry->next = NULL;
    }
	
    return first_entry;
}

void deleteTabEntryid(tab_entry **first_entry, int id_out)
{
    tab_entry *tmp;
	
    if((*first_entry)->id_dest == id_out)
    {
	tmp = *first_entry; 
        *first_entry = (*first_entry)->next;
        free(tmp);
    }
    else
    {
        tab_entry *curr = *first_entry;
		
	while(curr->next != NULL)
	{
	    if(curr->next->id_dest == id_out)
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp);
                break;
            }
            else
                curr = curr->next;
	}
    }
}

void deleteTabEntryfd(tab_entry **first_entry, int fd_out)
{
    tab_entry *tmp;
	
    if((*first_entry)->fd_sock == fd_out)
    {
	tmp = *first_entry; 
        *first_entry = (*first_entry)->next;
        free(tmp);
    }
    else
    {
	tab_entry *curr = *first_entry;
		
	while(curr->next != NULL)
	{
	    if(curr->next->fd_sock == fd_out)
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp);
                break;
            }
            else
                curr = curr->next;
        }
    }
}

void writeAdvtoEntryNode(tab_entry *first_entry, int errcode, char *buffer, int fd)
{
    tab_entry *aux = first_entry;
	
    while(aux != NULL)
    {
	errcode = snprintf(buffer, 150, "ADVERTISE %d\n",aux->id_dest);  
        if (buffer == NULL || errcode < 0 || errcode >= 150)
        {
            // isto tá mal, o strncpy não afeta o errno!!
	    // deixo por agora para me lembrar de mudar em todos
	    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
            exit(-1);  
        }
        writeTCP(fd, strlen(buffer), buffer);
        aux = aux->next;
    }
}

int checkTabEntry(tab_entry *first_entry, int id)
{
    tab_entry *aux = first_entry;
	
    while(aux != NULL)
    {
        if(aux->id_dest == id)
	{
	    return 1;
	}
	aux = aux->next;
    }
    return 0;
}

void printTabExp(tab_entry *first_entry)
{
    tab_entry *aux = first_entry;
    printf("Routing Table:\n");
    while(aux != NULL)
    {
	printf("%d, %d\n",aux->id_dest,aux->fd_sock);
	aux = aux->next;
    }
}

void FreeTabExp(tab_entry **first_entry)
{
    tab_entry *curr = *first_entry;
    tab_entry *next;
	
    while(curr != NULL)
    {
	next = curr->next;
	free(curr);
        curr = next;
    }
	
    *first_entry = NULL;
}

char *getConcatString( const char *str1, const char *str2 ) 
{
    char *finalString = NULL;
    size_t n = 0;

    if ( str1 ) n += strlen( str1 );
    if ( str2 ) n += strlen( str2 );

    if ( ( str1 || str2 ) && ( finalString = malloc( n + 1 ) ) != NULL )
    {
        *finalString = '\0';

        if ( str1 ) strcpy( finalString, str1 );
        if ( str2 ) strcat( finalString, str2 );
    }

    return finalString;
}

list_objects *createinsertObject(list_objects *head, char *subname, int id)
{
    int errcode;
    char *str;
    char str_id[150]; 
    list_objects *tmp = head;
    list_objects *new_obj = safeMalloc(sizeof(list_objects));
    
    memset(str_id, 0, 150);
    
    errcode = snprintf(str_id, 150, "%d.", id);
    if (str_id == NULL || errcode < 0 || errcode >= 150)
    {
         fprintf(stderr, "error in REG UDP message creation: %s\n", strerror(errno));
         exit(-1);
    }
    
    str = getConcatString(str_id, subname);
    
    new_obj->objct = safeMalloc(strlen(str)+1);
    // INSEGURO
    strcpy(new_obj->objct, str);
	
    free(str);
    //printf("%s:%d\n",new_obj->objct, strlen(new_obj->objct));

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

int checkCache(cache_objects cache[N], char *name, int n_obj)
{
    int i;
	
    for(i=0; i<n_obj; i++)
    {
	if(strcmp(cache[i].obj, name) == 0)
	{
	    return 1;
	}
    }
    return 0;
}

void saveinCache(cache_objects cache[N], char *name, int n_obj)
{
    if(n_obj > N)
    {
        free(cache[0].obj);
	cache[0].obj = safeMalloc(strlen(cache[1].obj));
	strcpy(cache[0].obj, cache[1].obj);
	free(cache[1].obj);
	n_obj--;
    }
    if(n_obj == 1)
    {
	cache[0].obj = safeMalloc(strlen(name)+1);
	strcpy(cache[0].obj, name);
    }
    if(n_obj == 2)
    {
	cache[1].obj = safeMalloc(strlen(name)+1);
	strcpy(cache[1].obj, name);
    }
}

void printCache(cache_objects cache[N], int n_obj)
{
    int i;
    printf("Cache\nNº Objetos armazenados: %d\n",n_obj);
    for(i=0; i<n_obj; i++)
    {
        printf("%s\n",cache[i].obj);
    }
}

int checkObjectList(list_objects *head_obj, char *name)
{
    list_objects *aux = head_obj;
	
    while(aux != NULL)
    {
	if(!strcmp(aux->objct, name))
	{
	    return 1;
	}
	aux = aux->next;
    }
    return 0;
}

void printObjectList(list_objects *head_obj)
{
    list_objects *aux = head_obj;
    while(aux != NULL)
    {
        printf("%s\n",aux->objct);
	aux = aux->next;
    }
}

void FreeObjectList(list_objects **head_obj)
{
    list_objects *curr = *head_obj;
    list_objects *next;
	
    while(curr != NULL)
    {
        next = curr->next;
        free(curr->objct);
	free(curr);
	curr = next;
    }
	
    *head_obj = NULL;
}

void addToList(internals **int_neighbours, viz *new)
{
    internals *aux = *int_neighbours;
    *int_neighbours = safeMalloc(sizeof(internals));
    (*int_neighbours)->this = new;
    (*int_neighbours)->this->flag_interest = 0;
    (*int_neighbours)->next = aux;
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
    int num_nodes, fd_udp, max_fd, counter, tcp_server_fd, we_are_reg=0;
    int errcode, external_is_filled;
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
    internals *int_neighbours = NULL, *neigh_aux, *neigh_tmp;
    
    tab_entry *first_entry = NULL, *tab_aux;
    cache_objects cache[N];
    int n_obj = 0;
    char *token1, *str_name;
    int ident;
    // estados associados ao select
    enum {not_waiting, waiting_for_list, waiting_for_regok, waiting_for_unregok} udp_state;
    udp_state = not_waiting;
    char waiting_for_backup = 0;
    // lista de mensagens recebidas num readTCP
    messages *msg_list, *msg_aux;
    no self;
    list_objects *head = NULL;

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
            printf("New connection mudafucka\n");
            new = safeMalloc(sizeof(viz));
            new->next_av_ix = 0;
            addrlen = sizeof(addr);
            if ((new->fd = accept(tcp_server_fd, &addr, &addrlen)) == -1)
            {
                fprintf(stderr, "Error accepting new TCP connection: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
			
	    writeAdvtoEntryNode(first_entry, errcode, message_buffer, new->fd);
			
            // se este processo estava em modo NONODES, ninguém nos deve contactar por TCP
            // porque não estamos ligados a qualquer rede
            if (network_state == NONODES)
            {
                printf("Não estamos em nenhuma rede\n");
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
                printf("Estamos sozinhos numa rede\n");
                network_state = TWONODES;
                external_is_filled = 0;
                // notar que apesar de estarmos em twonodes, não temos o external preenchido
                // provavelmente temos de colocar aqui alguma flag acerca disso, nem que seja
                // por causa do st, que vai ler do external que não está preenchido
            } 
            // se este processo estava em modo TWONODES ou MANYNODES, alguém que nos contacta
            // será mais um vizinho interno
            else if (network_state == TWONODES || network_state == MANYNODES)
            {
                printf("Estamos numa rede com 2 ou mais nós. Alguém se ligou, vamos torná-lo um vizinho interno\n");
                // isto é para o caso que está diretamente acima nos comentários, em que não temos os dados do external preenchidos
                // depois enviamos o EXTERN para este interno quando FD_ISSET(external->fd) e recebermos o NEW
                if (external_is_filled)
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
                }
                addToList(&int_neighbours, new);
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
                    
                    // sempre que recebemos EXTERN devemos atualizar a informação do nosso nó de backup
                    // pode ser quando fizemos connect e enviámos o NEW ou quando o nosso vizinho de backup
                    // fez leave e o nosso vizinho externo se ligou ao seu backup, atualizando o nosso backup
                    if (!strcmp(command, "EXTERN") && word_count == 3)
                    {
                        waiting_for_backup = 0;
                        printf("Just received the backup data\n");
                        strncpy(backup->IP, arg1, NI_MAXHOST);
                        strncpy(backup->port, arg2, NI_MAXSERV);
                        if (!we_are_reg)
                        {
                            // este é o caso em que nós fizemos connect e enviámos o NEW
                            // vamos armazenar a informação do nosso backup e registar os nossos
                            // dados no servidor de nós

                            // criar string para enviar o registo do nó
                            errcode = snprintf(message_buffer, 150, "REG %u %s %s", self.net, self.IP, self.port);  
                            if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                            {
                                fprintf(stderr, "error in REG UDP message creation: %s\n", strerror(errno));
                                exit(-1);
                            }
                            sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in REG UDP message send\n");
                            udp_state = waiting_for_regok;
                            printf("Acabámos de enviar o REG UDP\n");
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
                            // isto tá mal, o strncpy não afeta o errno!!
                            // deixo por agora para me lembrar de mudar em todos
                            fprintf(stderr, "error in EXTERN message creation when there are only two nodes: %s\n", strerror(errno));
                            exit(-1);
                        }
                        writeTCP(external->fd, strlen(message_buffer), message_buffer);
                        network_state = MANYNODES; // em princípio este vai sair
                        // nesta situação, o nosso backup vamos ser nós próprios
                        strncpy(backup->IP, argv[1], NI_MAXHOST); 
                        strncpy(backup->port, argv[2], NI_MAXSERV);

                        // enviar para todos os internos os dados do nosso vizinho externo
                        // isto porque caso estivéssemos sozinhos na rede e de repente se ligaram 
                        // vários gajos a nós por connect, podemos ter ficado numa situação em que
                        // aceitámos connect, estávamos MANYNODES mas não tínhamos o nosso vizinho externo preenchido
                        // com o seu IP e porto. Nesse caso, ao recebermos este NEW temos de notificar todos os internos
                        neigh_aux = int_neighbours;
                        while (neigh_aux)
                        {
                            writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
                            neigh_aux = neigh_aux->next;
                        }
                        
                    }
                    
                    // para quando recebemos uma mensagem ADVERTISE do vizinho externo, reencaminhar esta mensagem para os nossos internos (se houverem)
                    // no final deste processo adicionamos a entrada a nossa tabela de expedição     
		    if (!strcmp(command, "ADVERTISE") && word_count == 2)
                    {
		        neigh_aux = int_neighbours;
			while (neigh_aux != NULL)
			{
			    writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message);
			    neigh_aux = neigh_aux->next;
			}
						
			first_entry = createinsertTabEntry(first_entry, atoi(arg1), external->fd);
		    }
					
		    // para quando recebemos uma mensagem WITHDRAW do vizinho externo, reencaminhar esta mensagem para os nossos internos (se houverem)
		    // no final deste processo eliminamos a respetiva entrada da tabela de expedição correspondente ao id do nó que saiu da rede
		    if (!strcmp(command, "WITHDRAW") && word_count == 2)
		    {
			neigh_aux = int_neighbours;
			while (neigh_aux != NULL)
			{
			    writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message);
			    neigh_aux = neigh_aux->next;
			}
			deleteTabEntryid(&first_entry, atoi(arg1));
		    }
					
		    //para quando recebemos uma mensagem INTEREST do vizinho externo
		    if (!strcmp(command, "INTEREST") && word_count == 2)
		    {
			token1 = NULL; 
			ident = 0;
			str_name = NULL;
			str_name = safeMalloc(strlen(arg1)+1); 
			strcpy(str_name,arg1);
			token1 = strtok(str_name,".");
			ident = atoi(token1);
			free(str_name);
			
			// ativar flag_interest para o vizinho externo que nos mandou a mensagem de interesse
			external->flag_interest = 1;
						
			// verificar primeiro se o identificador do objeto corresponde ao nosso (se somos o destino da mensagem de interesse)    
			if(self.id == ident)
			{
			    // caso seja igual verificamos se temos o objeto na nossa lista de objetos
			    if(checkObjectList(head, arg1) == 1)
			    {
				// se tivermos o objeto na lista de objetos enviamos mensagem DATA de volta ao vizinho externo que nos mandou a mensagem de interesse
				errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
				if (message_buffer == NULL || errcode < 0 || errcode >= 150)
				{
				    // isto tá mal, o strncpy não afeta o errno!!
				    // deixo por agora para me lembrar de mudar em todos
				    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				    exit(-1);
				}
								
				writeTCP(external->fd, strlen(message_buffer), message_buffer);
			    }
			    else
			    {
				// se não tivermos o objeto na lista de objetos enviamos mensagem NODATA de volta ao vizinho externo que nos mandou a mensagem de interesse
				errcode = snprintf(message_buffer, 150, "NODATA %s\n", arg1);  
				if (message_buffer == NULL || errcode < 0 || errcode >= 150)
				{
				    // isto tá mal, o strncpy não afeta o errno!!
				    // deixo por agora para me lembrar de mudar em todos
				    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				    exit(-1);
				}
                        
			        writeTCP(external->fd, strlen(message_buffer), message_buffer);
			    }
			    external->flag_interest = 0;
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
				    // isto tá mal, o strncpy não afeta o errno!!
				    // deixo por agora para me lembrar de mudar em todos
				    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				    exit(-1);
				}
                        
				writeTCP(external->fd, strlen(message_buffer), message_buffer);
				external->flag_interest = 0;
			    }
			    else
			    {
				// se não tivermos o objeto na cache, não sendo nós o destino, reencaminhamos a mensagem INTEREST para o próximo nó através da tabela de expedição
				tab_aux = first_entry;
				while(tab_aux != NULL)
				{
				    if(tab_aux->id_dest == ident && tab_aux->fd_sock != SELFFD)
				    {
					writeTCP(tab_aux->fd_sock, strlen(msg_list->message), msg_list->message);
				    }
				    tab_aux = tab_aux->next;
			        }
			    }
		        }
		    }
					
		    // para quando recebemos uma mensagem DATA do vizinho externo, primeiro armazenamos o objeto na cache de objetos
		    // depois reencaminhamos a mensagem de volta para o vizinho de onde veio a mensagem de interesse (através da flag_interest)
		    // se a flag_interest do interno estiver desativada e se não se reencaminhar nenhuma mensagem 
		    // isto quer dizer que a mensagem DATA chegou ao nó que tinha enviado a primeira mensagem INTEREST inicial da pesquisa
		    if (!strcmp(command, "DATA") && word_count == 2)
		    {
			n_obj++;
			if(n_obj <= N)
			{
			    saveinCache(cache, arg1 ,n_obj);
			}
			else
			{
			    saveinCache(cache, arg1, n_obj);
			    n_obj--;
			}
					
			neigh_aux = int_neighbours;
			while (neigh_aux != NULL)
			{
			    if(neigh_aux->this->flag_interest == 1)
			    {
				writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message);
				neigh_aux->this->flag_interest = 0;
			    }
			    neigh_aux = neigh_aux->next;
		        }
		    }
					
		    // para quando recebemos uma mensagem NODATA do vizinho externo, NÃO PRECISAMOS de armazenar nada na cache de objetos
		    // simplesmente reencaminhamos a mensagem de volta para o vizinho de onde veio a mensagem de interesse (através da flag_interest)
		    // se a flag_interest do interno estiver desativada e se não se reencaminhar nenhuma mensagem 
		    // isto quer dizer que a mensagem NODATA chegou ao nó que tinha enviado a mensagem INTEREST inicial da pesquisa
		    if (!strcmp(command, "NODATA") && word_count == 2)
		    {	
			neigh_aux = int_neighbours;
			while (neigh_aux != NULL)
			{
			    if(neigh_aux->this->flag_interest == 1)
			    {
				writeTCP(neigh_aux->this->fd, strlen(msg_list->message), msg_list->message);
			        neigh_aux->this->flag_interest = 0;
			    }
			    neigh_aux = neigh_aux->next;
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
	        tab_aux = first_entry;
		while(tab_aux != NULL)
		{
		    if(tab_aux->fd_sock == external->fd)
		    {
		        errcode = snprintf(message_buffer, 150, "WITHDRAW %d\n", tab_aux->id_dest);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
			    fprintf(stderr, "error in WITHDRAW message creation when there are only two nodes: %s\n", strerror(errno));
                            exit(-1);
			}
			neigh_aux = int_neighbours;
			while (neigh_aux != NULL)
			{
			    writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
			    neigh_aux = neigh_aux->next;
			}
			deleteTabEntryfd(&first_entry, tab_aux->fd_sock);
		     }
		     tab_aux = tab_aux->next;
	    	}

                if ((errcode = close(external->fd)))
                {
                    fprintf(stderr, "error closing external fd when external closes connection: %s\n", strerror(errno));
                    exit(-1);
                }
                // neste caso nós somos o nosso próprio vizinho de backup
                if (!strcmp(self.IP, backup->IP) && !strcmp(self.port, backup->port))
                {
                    printf("Fomos abandonados pelo nosso externo e éramos o nosso backup\n");
                    free(external);
                    // neste caso vamos promover um dos vizinhos internos a vizinho externo
                    // o vizinho que vai ser promovido vai ser o do topo da lista
                    if (int_neighbours)
                    {
                        external = int_neighbours->this;
                        neigh_aux = int_neighbours;
                        int_neighbours = int_neighbours->next;     
                        free(neigh_aux);
                        neigh_aux = int_neighbours;
                        // enviar um EXTERN com os dados do vizinho interno promovido a vizinho externo
                        // para o notificar  
                        errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            // isto tá mal, o strncpy não afeta o errno!!
                            // deixo por agora para me lembrar de mudar em todos
                            fprintf(stderr, "error in EXTERN message creation when there are only two nodes: %s\n", strerror(errno));
                            exit(-1);
                        }
                        writeTCP(external->fd, strlen(message_buffer), message_buffer);
                        // para todos os vizinhos internos que não foram promovidos a vizinhos externos, notificá-los também
                        // que têm um novo vizinho externo
                        while (neigh_aux)
                        {
                            writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
                            neigh_aux = neigh_aux->next;
                        }
                    }
                    // caso fôssemos o nosso próprio backup e não tivéssemos vizinhos 
                    // internos, então vamos sozinhos numa rede
                    else
                    {
                        printf("Não tínhamos vizinhos internos\n");
                        network_state = ONENODE;
                        external = NULL;
                    }
                }
                
                // neste caso temos um backup, tornamos esse backup o nosso externo
                else
                {
                    
                    // atualizar a informação do vizinho externo
                    strncpy(external->IP, backup->IP, NI_MAXHOST);
                    strncpy(external->port, backup->port, NI_MAXSERV);
                    external->next_av_ix = 0;
                    safeTCPSocket(&(external->fd));
                    // ligar ao vizinho de recuperação (futuro vizinho externo)
                    connectTCP(external->IP, external->port, external->fd, 
                               "Error getting address info for external node when previous external closes connection\n", 
                               "Error connecting to external node when previous external closes connection\n");

                    // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
                    errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self.IP, self.port);  
                    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                    {
                        fprintf(stderr, "error in NEW TCP message creation when previous external closes connection: %s\n", strerror(errno));
                        exit(-1);
                    }
                    writeTCP(external->fd, strlen(message_buffer), message_buffer);
                    waiting_for_backup = 1; // we're outnumbered, need backup

		    //se tivermos internos que não sabem ainda que o nosso externo (o seu backup) mudou, notificá-los através da mensagem EXTERN
                    if(int_neighbours)
                    {
			errcode = snprintf(message_buffer, 150, "EXTERN %s %s\n", external->IP, external->port);  
                        if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                        {
                            // isto tá mal, o strncpy não afeta o errno!!
                            // deixo por agora para me lembrar de mudar em todos
                            fprintf(stderr, "error in EXTERN message creation when there are only two nodes: %s\n", strerror(errno));
                            exit(-1);
                        }
                        neigh_aux = int_neighbours;
                        while (neigh_aux != NULL)
                        {
                            writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
                            neigh_aux = neigh_aux->next;
                        }
		    }
                    // possivelmente deixar alguma informação na estrutura do backup que está desatualizado
                    // mas como estamos waiting_for_backup, podemos deduzir isso daí
                    writeAdvtoEntryNode(first_entry, errcode, message_buffer, external->fd);
                }
                
            }
        }
        // mudar isto tudo
        neigh_aux = int_neighbours; // neigh_aux irá apontar para o elemento atual da lista
        neigh_tmp = NULL; // neigh_tmp irá apontar para o vizinho interno prévio para motivos de remoção de um elemento da lista
        while (neigh_aux != NULL)
        {
            printf("Estamos a iterar pela lista de vizinhos internos\n");
            // de momento de um vizinho interno só recebemos o new
            if (FD_ISSET(neigh_aux->this->fd, &rfds))
            {
                printf("Este vizinho interno tem um read que não bloqueia\n");
                if ((tcp_read_flag = readTCP(neigh_aux->this)) == MSG_FINISH)
                {
                    printf("Este vizinho interno enviou uma mensagem completa\n");
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
			    neigh_tmp = int_neighbours;
			    while (neigh_tmp != NULL)
			    {
				if(neigh_tmp->this->fd != neigh_aux->this->fd)
				{
				    writeTCP(neigh_tmp->this->fd, strlen(msg_list->message), msg_list->message);
				}
			        neigh_tmp = neigh_tmp->next;
			    }
						
			    writeTCP(external->fd, strlen(msg_list->message), msg_list->message);
		
			    first_entry = createinsertTabEntry(first_entry, atoi(arg1), neigh_aux->this->fd);
			}
						
			// para quando recebemos uma mensagem WITHDRAW de um interno, reencaminhar esta mensagem para os outros internos (se houverem), bem como ao nosso externo
			// no final deste processo eliminamos a respetiva entrada da tabela de expedição correspondente ao id do nó que saiu da rede
			if (!strcmp(command, "WITHDRAW") && word_count == 2)
			{
			    neigh_tmp = int_neighbours;
			    while (neigh_tmp != NULL)
			    {
				if(neigh_tmp->this->fd != neigh_aux->this->fd) 
				{
				    writeTCP(neigh_tmp->this->fd, strlen(msg_list->message), msg_list->message);
				}
			        neigh_tmp = neigh_tmp->next;
			    }
							
			    writeTCP(external->fd, strlen(msg_list->message), msg_list->message);
							
			    deleteTabEntryid(&first_entry, atoi(arg1));
			}
						
			// para quando recebemos uma mensagem INTEREST dum vizinho interno
                        if (!strcmp(command, "INTEREST") && word_count == 2)
		        {
		            token1 = NULL; 
			    ident = 0;
			    str_name = NULL;
			    str_name = safeMalloc(strlen(arg1)+1); 
			    strcpy(str_name,arg1);
			    token1 = strtok(str_name,".");
			    ident = atoi(token1);
			    free(str_name);
				
			    // ativar flag_interest para o vizinho interno que nos mandou a mensagem de interesse
			    neigh_aux->this->flag_interest = 1;
				
			    // verificar primeiro se o identificador do objeto corresponde ao nosso (se somos o destino da mensagem de interesse)		
			    if(self.id == ident)
			    {
				// caso seja igual verificamos se temos o objeto na nossa lista de objetos
				if(checkObjectList(head, arg1) == 1)
				{
				    // se tivermos o objeto na lista de objetos enviamos mensagem DATA de volta ao vizinho interno que nos mandou a mensagem de interesse
				    errcode = snprintf(message_buffer, 150, "DATA %s\n", arg1);  
				    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
				    {
					// isto tá mal, o strncpy não afeta o errno!!
					// deixo por agora para me lembrar de mudar em todos
					fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
					exit(-1);
				    }
									
				    writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
				}
				else
				{
				    // se não tivermos o objeto na lista de objetos enviamos mensagem NODATA de volta ao vizinho interno que nos mandou a mensagem de interesse
				    errcode = snprintf(message_buffer, 150, "NODATA %s\n", arg1);  
				    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
				    {
					// isto tá mal, o strncpy não afeta o errno!!
					// deixo por agora para me lembrar de mudar em todos
					fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				        exit(-1);
				    }
                        
				    writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
				}
			        neigh_aux->this->flag_interest = 0;
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
					// isto tá mal, o strncpy não afeta o errno!!
					// deixo por agora para me lembrar de mudar em todos
					fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				        exit(-1);
				    }
                        
				    writeTCP(neigh_aux->this->fd, strlen(message_buffer), message_buffer);
				    neigh_aux->this->flag_interest = 0;
				}
				else
				{
				    // se não tivermos o objeto na cache, não sendo nós o destino, reencaminhamos a mensagem INTEREST para o próximo nó através da tabela de expedição
				    tab_aux = first_entry;
				    while(tab_aux != NULL)
				    {
					if(tab_aux->id_dest == ident && tab_aux->fd_sock != SELFFD)
					{
					    writeTCP(tab_aux->fd_sock, strlen(msg_list->message), msg_list->message);
					}
				        tab_aux = tab_aux->next;
				    }
			        }
			    }
			}
						
			// para quando recebemos uma mensagem DATA do vizinho interno, primeiro armazenamos o objeto na cache de objetos
			// depois reencaminhamos a mensagem para o vizinho de onde veio a mensagem de interesse (através da flag_interest)
			// se as flag_interest do externo e interno estiverem ambas desativadas e se não se reencaminhar nenhuma mensagem 
			// isto quer dizer que a mensagem DATA chegou ao nó que tinha enviado a mensagem INTEREST inicial da pesquisa
			if (!strcmp(command, "DATA") && word_count == 2)
			{
			    n_obj++;
			    if(n_obj <= N)
			    {
				saveinCache(cache, arg1 ,n_obj);
			    }
			    else
			    {
				saveinCache(cache, arg1, n_obj);
				n_obj--;
			    }
	 
			    neigh_tmp = int_neighbours;
			    while (neigh_tmp != NULL)
			    {
				if(neigh_tmp->this->fd != neigh_aux->this->fd && neigh_tmp->this->flag_interest == 1)
				{
				    writeTCP(neigh_tmp->this->fd, strlen(msg_list->message), msg_list->message);
				    neigh_tmp->this->flag_interest = 0;
				    external->flag_interest = 0;
				}
			        neigh_tmp = neigh_tmp->next;
			    }

			    if(external->flag_interest == 1)
			    {
			        writeTCP(external->fd, strlen(msg_list->message), msg_list->message);
			    }
			}
						
			// para quando recebemos uma mensagem NODATA do vizinho interno, NÃO PRECISAMOS de armazenar nada na cache de objetos
			// simplesmente reencaminhamos a mensagem de volta para o vizinho de onde veio a mensagem de interesse (através da flag_interest)
			// se as flag_interest do externo e interno estiverem ambas desativadas e se não se reencaminhar nenhuma mensagem 
			// isto quer dizer que a mensagem DATA chegou ao nó que tinha enviado a mensagem INTEREST inicial da pesquisa
			if (!strcmp(command, "NODATA") && word_count == 2)
			{
			    neigh_tmp = int_neighbours;
			    while (neigh_tmp != NULL)
			    {
				if(neigh_tmp->this->fd != neigh_aux->this->fd && neigh_tmp->this->flag_interest == 1)
				{
				    writeTCP(neigh_tmp->this->fd, strlen(msg_list->message), msg_list->message);
				    neigh_tmp->this->flag_interest = 0;
				    external->flag_interest = 0;
				}
				neigh_tmp = neigh_tmp->next;
			    }

			    if(external->flag_interest == 1)
			    {
			        writeTCP(external->fd, strlen(msg_list->message), msg_list->message);
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
                    neigh_tmp = neigh_aux;
                    neigh_aux = neigh_aux->next;
                }
                // vizinho interno fez leave e fechou as conexões
                else if (tcp_read_flag == MSG_CLOSED)
                {
		    tab_aux = first_entry;
		    while(tab_aux != NULL)
		    {
		        if(tab_aux->fd_sock == neigh_aux->this->fd)
			{
			    errcode = snprintf(message_buffer, 150, "WITHDRAW %d\n", tab_aux->id_dest);  
			    if (message_buffer == NULL || errcode < 0 || errcode >= 150)
			    {
				fprintf(stderr, "error in WITHDRAW message creation when there are only two nodes: %s\n", strerror(errno));
				exit(-1);
			    }
			    neigh_tmp = int_neighbours;
			    while (neigh_tmp != NULL)
			    {
			        if(neigh_tmp->this->fd != neigh_aux->this->fd) 
				{
				    writeTCP(neigh_tmp->this->fd, strlen(message_buffer), message_buffer);
				}
				neigh_tmp = neigh_tmp->next;
			    }
							
			    writeTCP(external->fd, strlen(message_buffer), message_buffer);
			    deleteTabEntryfd(&first_entry, tab_aux->fd_sock);
			}
			tab_aux = tab_aux->next;
		    }
					
                    close(neigh_aux->this->fd); // fechar o fd do tipo que fez close do outro lado
                    printf("O nosso vizinho interno abandonou\n");
                    // remover o vizinho interno da lista
                    
                    // neste caso, o nó que vamos remover da lista
                    // era o topo da lista
                    if (neigh_tmp == NULL)
                    {
                        printf("O nosso vizinho interno estava no topo da lista\n");
                        int_neighbours = int_neighbours->next;
                        if (!int_neighbours)
                            printf("De facto, o int_neighbours->next estava a NULL e ficámos sem vizinhos internos\n");
                        // limpar a memória do vizinho interno removido        
                        free(neigh_aux->this);
                        free(neigh_aux);
                        neigh_aux = NULL;

                        // o nó prévio fica a NULL, não é preciso alterar nada
             
                        // o nó por onde vamos iterar a seguir é o novo topo da lista
                        neigh_aux = int_neighbours;
                    }
                    // neste caso o nó que vamos remover da lista
                    // estava ou no meio ou na cauda
                    else
                    {
                        // ligar o nosso predecessor (neigh_tmp) ao nosso sucessor (neigh_aux->next)
                        neigh_tmp->next = neigh_aux->next;
                        // limpar a memória do vizinho interno removido        
                        free(neigh_aux->this);
                        free(neigh_aux);
                        // o nó por onde vamos iterar a seguir é o sucessor do nó que acabámos de remover (que está guardado em neigh_tmp->next)
                        neigh_aux = neigh_tmp->next;
                    }
                    
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
		    first_entry = createinsertTabEntry(first_entry, self.id, SELFFD);
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
                        printf("%d\n",num_nodes);
                        external = safeMalloc(sizeof(viz));
                        external->next_av_ix = 0;
                        safeTCPSocket(&(external->fd));
                        connectTCP(nodes_fucking_list->IP, nodes_fucking_list->port, external->fd, 
                                "Error getting address info for external node in JOIN\n", "Error connecting to external node in JOIN\n");

                        waiting_for_backup = 1; // we're outnumbered, need backup
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
                        // aqui devíamos colocar um estado novo em que ficamos à espera do extern
                         
                        // acabar de preencher a informação do external
                        strncpy(external->IP, nodes_fucking_list->IP, NI_MAXHOST);
                        strncpy(external->port, nodes_fucking_list->port, NI_MAXSERV);
                        
                        external_is_filled = 1;
                        
                        errcode = snprintf(message_buffer, 150, "ADVERTISE %d\n",self.id);  
			if (message_buffer == NULL || errcode < 0 || errcode >= 150)
			{
				// isto tá mal, o strncpy não afeta o errno!!
				// deixo por agora para me lembrar de mudar em todos
				fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				exit(-1);
			}
                        
			writeTCP(external->fd, strlen(message_buffer), message_buffer);
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
                strncpy(self.IP, argv[1], NI_MAXHOST);
                strncpy(self.port, argv[2], NI_MAXSERV);
                
                // criar string para enviar o pedido de nós
                errcode = snprintf(message_buffer, 150, "NODES %u", self.net);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 100)
                {
                    fprintf(stderr, "error in JOIN UDP message creation: %s\n", strerror(errno));
                    exit(-1);
                }
                sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in JOIN UDP message send\n");
                udp_state = waiting_for_list;
				external_is_filled = 0;
            }
            else if (instr_code == JOIN_LINK && network_state == NONODES)
            {
                // esta condição em princípio nunca será necessária
                // visto que quando iniciamos o programa o external é colocado a NULL
                // e quando fazemos LEAVE também. Portanto temos sempre de alocar memória
                if (!external)
                    external = safeMalloc(sizeof(viz));
                external_is_filled = 1;
                if(sscanf(user_input,"%u %d %s %s",&self.net,&self.id, external->IP, external->port ) != 4)
                {
                    printf("Error in sscanf JOIN_LINK\n");
                    exit(1);
                }
                strncpy(self.IP, argv[1], NI_MAXHOST);
                strncpy(self.port, argv[2], NI_MAXSERV);

                external->next_av_ix = 0;
                safeTCPSocket(&(external->fd));
                connectTCP(external->IP, external->port, external->fd, 
                        "Error getting address info for external node in JOIN_LINK\n", "Error connecting to external node in JOIN_LINK\n");

                waiting_for_backup = 1; // we're outnumbered, need backup
                network_state = TWONODES; // pelo menos até recebermos a informação do backup, não sabemos se não há apenas 2 nodes
                // quer dizer, podemos ver pelo num_nodes na verdade

                // enviar mensagem new, com a informação do IP/porto do nosso servidor TCP 
                errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self.IP, self.port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in NEW TCP message creation in JOIN_LINK: %s\n", strerror(errno));
                    exit(-1);
                }
                writeTCP(external->fd, strlen(message_buffer), message_buffer);
                
                first_entry = createinsertTabEntry(first_entry, self.id, SELFFD);
                
                errcode = snprintf(message_buffer, 150, "ADVERTISE %d\n",self.id);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
		    // isto tá mal, o strncpy não afeta o errno!!
		    // deixo por agora para me lembrar de mudar em todos
		    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
		    exit(-1);
		}
                        
		writeTCP(external->fd, strlen(message_buffer), message_buffer);
            }
            else if(instr_code == JOIN_SERVER_DOWN && network_state == NONODES)
            {
                network_state = ONENODE;
                we_are_reg = 1;

                if(sscanf(user_input,"%u %d",&self.net,&self.id) != 2)
                {
                    printf("Error in sscanf JOIN_LINK\n");
                    exit(1);
                }
                strncpy(self.IP, argv[1], NI_MAXHOST);
                strncpy(self.port, argv[2], NI_MAXSERV);
                
                first_entry = createinsertTabEntry(first_entry, self.id, SELFFD);
            }
            else if (instr_code == LEAVE && network_state != NONODES)
            {
                we_are_reg = 0;
                // criar string para enviar o desregisto (?isto é uma palavra) do nó
                errcode = snprintf(message_buffer, 150, "UNREG %u %s %s", self.net, self.IP, self.port);  
                if (message_buffer == NULL || errcode < 0 || errcode >= 150)
                {
                    fprintf(stderr, "error in UNREG UDP message creation: %s\n", strerror(errno));
                    exit(-1);
                }
                sendUDP(fd_udp, argv[3], argv[4], message_buffer, "Error getting address information for UDP server socket\n", "error in UNREG UDP message send\n");
                udp_state = waiting_for_unregok;
               
                // após tirar o registo no servidor de nós, devemos terminar todas as conexões TCP ativas 
                
                // terminar a sessão TCP com o vizinho externo
                // esta condição é importante para o caso em que fazemos leave e estávamos sozinhos
                // numa rede
                if (external)
                {
                    errcode = close(external->fd);
                    if (errcode)
                    {
                        fprintf(stderr, "error closing file descriptor of external neighbour: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    // deixar a variável external pronta para um possível JOIN vindouro
                    free(external);
                    external = NULL;
                }
                // terminar todas as conexões com vizinhos internos
                // e limpar a memória da lista
                while (int_neighbours)
                {
                    neigh_aux = int_neighbours;
                    int_neighbours = int_neighbours->next;
                    errcode = close(neigh_aux->this->fd);
                    if (errcode)
                    {
                        fprintf(stderr, "error closing file descriptor of internal neighbour: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    free(neigh_aux->this);
                    free(neigh_aux);
                    neigh_aux = NULL;
                }
                
                FreeTabExp(&first_entry);
                // indicar que não estamos ligados a qualquer rede
                network_state = NONODES;
            }
            else if (instr_code == CREATE && network_state != NONODES)
            {
		user_input[strlen(user_input)-1] = '\0';
		//printf("%s:%d\n",user_input, strlen(user_input));
                head = createinsertObject(head,user_input,self.id);
                printObjectList(head);
            }
            else if (instr_code == GET && network_state != NONODES)
            {
	        user_input[strlen(user_input)-1] = '\0';
		token1 = NULL; 
		ident = 0;
		str_name = safeMalloc(strlen(user_input)+1); 
		strcpy(str_name,user_input);
		token1 = strtok(str_name,".");
		ident = atoi(token1);
		//printf("%d\n%s\n",ident,token2);
		//printf("%d\n",strlen(user_input));
		free(str_name);
				
		if(checkCache(cache, user_input, n_obj) == 1)
		{
		    printf("Object %s found in our own cache!\n", user_input);
		}
		else
		{
		    if(checkTabEntry(first_entry,ident) == 1)
		    {
		        tab_aux = first_entry;
			while(tab_aux != NULL)
			{
			    if(tab_aux->id_dest == ident && tab_aux->fd_sock == SELFFD)
			    {
			        if(checkObjectList(head, user_input) == 1)
				{
				    printf("Object %s found in our own list of objects!\n", user_input);
				}
				else
				{
				    printf("Object %s not found!\n", user_input);
				}
			    }
			    if(tab_aux->id_dest == ident && tab_aux->fd_sock != SELFFD)
			    {
			        errcode = snprintf(message_buffer, 150, "INTEREST %s\n", user_input);  
				if (message_buffer == NULL || errcode < 0 || errcode >= 150)
				{
				    // isto tá mal, o strncpy não afeta o errno!!
				    // deixo por agora para me lembrar de mudar em todos
				    fprintf(stderr, "error in ADVERTISE message creation when there are only two nodes: %s\n", strerror(errno));
				    exit(-1);
				}
                        
				writeTCP(tab_aux->fd_sock, strlen(message_buffer), message_buffer);
			    }
			    tab_aux = tab_aux->next;
			}
		    }
		    else
		    {
		        printf("Node with the identifier %d does not exist in the network!\n",ident);
		    }
		}    
	    }
            else if(instr_code == EXIT)
                exit(0);
            else if(instr_code == ST)
            {
                if (network_state == NONODES)
                    printf("We're not connected to any network\n");
                else if (network_state == ONENODE)
                    printf("We're alone in a network\n");
                else if (network_state == TWONODES)
                {
                    printf("State is TWONODES\n");
                    printf("Is internal_neighbours null: %s\n", int_neighbours?"no":"yes");
                    printf("External neighbour's contact: %s:%s\n", external->IP, external->port);
                    if (!waiting_for_backup)
                        printf("Backup's contact: %s:%s\n", backup->IP, backup->port);
                    else 
                        printf("We are still waiting for the contact information of our backup node\n");
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
	    else if(instr_code == SC)
            {
		printCache(cache,n_obj);		
	    }
	    else if(network_state == NONODES && (instr_code == SR || instr_code == GET))
	    {
		printf("We're not connected to any network\n");
	    }
	    else if(network_state != NONODES && (instr_code == JOIN_ID || instr_code == JOIN_LINK))
	    {
	 	printf("We're already connected to a network\n");
            }
            free(user_input);
        }
    }	
    return 0;
}
