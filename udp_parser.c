#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp_parser.h"
#include "errcheck.h"
#include "nodes.h"
#include "tcp.h"

#define OK_READ 5
#define TIMER_EXPIRED 6
#define DGRAM_RECEIVED 7
extern int errno;

int parseNodeListRecursive(char* datagram, node_list **list, int *num_nodes)
{
    int rvalue;
    node_list *this = malloc(sizeof(node_list));
    if(this == NULL)
        return END_EXECUTION;
    
    this->next = NULL;
    rvalue = sscanf(datagram,"%s %s\n", this->IP, this->port);
    // um EOF indica muito provavelmente um erro no sscanf mesmo (visto que estamos a ler para string)
    // Então fechamos o programa
    if (rvalue == EOF)
    {
        free(this);
        fprintf(stderr, "error in parseNodeListRecursive() sscanf: %s\n", strerror(errno)); 
        return END_EXECUTION;
    }
    // neste caso houve um erro mas devido a uma mensagem mal formatada
    // sendo assim vamos sinalizar esse erro, mas não paramos o programa
    if(rvalue != 2){
        free(this);
        return LEAVE_NETWORK;
    }
    // neste caso conseguimos alocar a memória e armazenar o contacto do nó anunciado
    *num_nodes = (*num_nodes) + 1;
    *list = this;
    int ix =0;
    char c = datagram[ix];
    // obrigatoriamente tem um \n devido ao sscanf, se aquilo
    // não tiver bugs esquisitos com whitespace ser igual todo
    // um ao outro e o \n não existir mas estar la outro espaço qualquer, ou nenhum
    while ((c=datagram[ix]) != '\n')
        ix++;
    // chegámos ao fim da linha atual
    // se o próximo caractere for \0, lemos o dgram todo
    // caso contrário, vamos ler mais uma linha
    if (datagram[ix + 1] != '\0')
        // colocamos &(this->next) para que o próximo nó contendo informação de 
        // contacto apareça em this->next. Isto facilita caso haja um erro 
        // a meio da leitura da lista: aí limpamos a memória do nó onde houve o erro
        // em free(this) e depois na função que chamar esta usamos a freeNodesList
        // para limpar os nós anteriores àquele que deu erro
        return parseNodeListRecursive(datagram + ix + 1, &(this->next), num_nodes);
    
    return NO_ERROR;
}


// datagram é a mensagem recebida, terminada com \0
// net é o número da rede para a qual estamos à espera de receber a lista dos nós
// nodeslist_received é um booleano
char* isNodesList(char* datagram, char *net, char *nodeslist_received, int *error_flag){
    char message_until_newline[100]; // passar o 100 para uma constante, depois passar o 100
    char message_supposed_to_recv[100];
    *nodeslist_received = 0;
    int errcode;
    errcode = snprintf(message_supposed_to_recv, 100,  "NODESLIST %s\n", net);
    // na especificação do C em caso de erros ele devolve valores negativos
    // segundo a man page. A verificação para
    // ser maior de 100 é porque, caso houvesse mais de 100 caracteres para escrever,
    // ele vai devolver o número total que iria escrever se houvesse espaço
    if (errcode < 0 )
    {
        fprintf(stderr, "error in isNodesList() snprintf. It returned a negative value\n");
        *error_flag = END_EXECUTION;
        return NULL;
    }
    else if (errcode >= 100)
    {
        fprintf(stderr, "error in isNodesList() snprintf. The message would have more than 100 characters (including '/0'). Consider increasing the size of the buffer\n");
        *error_flag = LEAVE_NETWORK;
        return NULL;
    }
    char c = datagram[0];
    unsigned int ix=0;
    while(c != '\0')
    {
        message_until_newline[ix] = c;
        if (c == '\n')
        {
            message_until_newline[ix+1] = '\0';
            // se a primeira linha lida foi a do comando recebemos a mensagem correta
            if (!(errcode = strcmp(message_until_newline, message_supposed_to_recv))){
                *nodeslist_received = 1;
                *error_flag = NO_ERROR;
                // se ainda há mais linhas para ler, então a lista não vem vazia
                // nesse caso, devolvemos uma string que aponta para o início
                // da próxima linha. Caso contrário devolvemos NULL. Graças à flag
                // nodes_list_received conseguimos perceber se o NULL se deve a uma
                // lista vazia ou a uma mensagem mal formatada
                if (datagram[ix + 1] != '\0')
                    return datagram + ix + 1;
                return NULL;
            }
            break; // a mensagem vinha mal formatada
        }
        ix = ix + 1;
        // caso haja 99 caracteres antes do \n, então assumimos que a mensagem estava mal formatada, ou nao era um
        // NODESLIST
        // pode parecer estranho 100 - 1: o -1 aparece porque para além do próximo caractere, temos sempre de escrever
        // o \0. Logo se o ix é 100 - 1, se o próximo caractere terminasse a mensagem teríamos de escrever no ix 100 -1 (ainda válido) o \n 
        // e no 100 para meter o \0, e excederiamos a buffer
        if (ix == 100 - 1)
            break;
        c = datagram[ix];
    }
    // chegámos ao \0 mas não chegámos a 100 caracteres nem ao \n, ou chegámos a 99 caracteres antes do \n, ou chegámos ao \n 
    // mas não recebemos um NODESLIST bem formatado
    *error_flag = LEAVE_NETWORK;
    return NULL;
}

void freeNodeList(node_list **list)
{
    node_list *aux;
    while(*list != NULL)
    {
        aux = *list;
        *list = (*list)->next;
        free(aux);
    }
}

int sendAndWait(int fd_udp, struct timeval *tv, char* send_error_msg, char *send_IP, char *send_UDP, char *message_buffer)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    int counter;
    // enviar mensagem ao servidor
    if (sendUDP(fd_udp, send_IP, send_UDP, message_buffer, "Error getting address information for UDP server socket\n", send_error_msg) == ERROR)
        return NON_FATAL_ERROR;
    // esperar por resposta, ou timeout
    FD_SET(fd_udp, &rfds);
    counter = select(fd_udp+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, tv);
    if (counter == -1)
    {
        fprintf(stderr, "Error in select for UDP communication: %s\n", strerror(errno));
        return END_EXECUTION;
    }
    if (counter == 0)
    {
        return TIMER_EXPIRED;
    }
    return OK_READ;
}

int getDgram(char *send_error_msg, char *send_IP, char *send_UDP, char *dgram)
{
    int errcode, fd_udp, num_timeouts = 0;
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    // criar socket para comunicação UDP
    if ((fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        fprintf(stderr, "Error getting socket for UDP client: %s\n", strerror(errno));
        return END_EXECUTION;
    }
    while (num_timeouts < 3)
    {
        printf("Sending message via UDP\n");
        errcode = sendAndWait(fd_udp, &tv, send_error_msg, send_IP, send_UDP, dgram);
        if (errcode == TIMER_EXPIRED)
        {
            printf("We didn't receive a response from the UDP server\n");
            num_timeouts++;
            // valor escolhido aleatoriamente. Primeira transmissão espera 3 s, 2ª espera 6s, 3ª espera 9s
            tv.tv_sec = (1+num_timeouts)*3;
        }
        // se o timer não expirou, seja por um erro, seja por termos recebido a mensagem, vamos
        // parar de transmitir
        else 
            break;
    }
    if (errcode == OK_READ)
    {
        // ler a resposta do servidor
        errcode = safeRecvFrom(fd_udp, dgram, 999);
    }

    // fechar o fd associado à comunicação por UDP
    if (close(fd_udp) == -1)
        fprintf(stderr, "Error closing socket for UDP client: %s\n", strerror(errno));
    // o errcode pode ter sido colocado ou pelo sendAndWait ou pelo safeRecvFrom
    // de um modo ou de outro
    if (errcode == NON_FATAL_ERROR || errcode == TIMER_EXPIRED)
        return NON_FATAL_ERROR;
    else if (errcode == END_EXECUTION)
        return END_EXECUTION;
    return DGRAM_RECEIVED;
}
 
int unreg(no *self, char *send_IP, char *send_UDP)
{
    // criar string para enviar o unreg
    char dgram[1000];
    int errcode = snprintf(dgram, 1000, "UNREG %s %s %s", self->net, self->IP, self->port);  
    if (errcode < 0)
    {
        fprintf(stderr, "error in UNREG UDP message creation: snprintf returned a negative value\n");
        return END_EXECUTION;
    }
    if (errcode >= 1000)
    {
        fprintf(stderr, "error in UNREG UDP message creation: snprintf needed to write past the buffer. Consider increasing the buffer's size\n");
        return LEAVE_NETWORK;
    }
    
    // se houve algum tipo de erro, devolvê-lo à main
    //
    if ((errcode = getDgram("error in UNREG UDP message send\n", send_IP, send_UDP, dgram)) != DGRAM_RECEIVED)
        return errcode;
    if (!strcmp(dgram, "OKUNREG"))
        printf("We received the confirmation of unregistration from the server\n");
    else
        fprintf(stderr, "We received gibberish via UDP\n");
    return NO_ERROR;
}

int getNodesList(char *self_net, char *send_IP, char *send_UDP, char *dgram)
{
    // criar string para enviar o pedido de nós
    int errcode = snprintf(dgram, 1000, "NODES %s", self_net);  
    if (errcode < 0)
    {
        fprintf(stderr, "error in NODES UDP message creation: snprintf returned a negative value\n");
        return END_EXECUTION;
    }
    if (errcode >= 150)
    {
        fprintf(stderr, "error in NODES UDP message creation: snprintf needed to write past the buffer. Consider increasing the buffer's size\n");
        return LEAVE_NETWORK;
    }
    
    // se houve algum tipo de erro, devolvê-lo à main
    if ((errcode = getDgram("error in JOIN UDP message send\n", send_IP, send_UDP, dgram)) != DGRAM_RECEIVED)
        return errcode;
    return NO_ERROR;
}

int setListedNodeAsExternal(viz *external, node_list *aux_list_of_nodes, no *self, char *message_buffer)
{
    external->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (external->fd == -1){
        fputs("Error making a TCP socket for external neighbour after receiving nodes list!\n", stderr);
        fprintf(stderr, "error: %s\n", strerror(errno));
        return END_EXECUTION;
    }
    int errcode = connectTCP(aux_list_of_nodes->IP, aux_list_of_nodes->port, external->fd, 
            "Error getting address info for external node in JOIN\n", "Error connecting to external node in JOIN\n");
    // neste caso o errcode é NO_ERROR se o connect funcionou
    // tentamos enviar as 2 mensagens por TCP que temos de enviar
    // se alguma delas falhar no envio, consideramos que não nos conseguimos ligar e tentamo-nos ligar ao seguinte nó da lista recebida
    if (errcode == NO_ERROR)
    {
        // enviar o NEW
        if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == NO_ERROR)
        {
            errcode = snprintf(message_buffer, 150, "ADVERTISE %s\n",self->id);  
            if (errcode < 0)
            {
                fprintf(stderr, "error in ADVERTISE TCP message creation. snprintf() returned a negative value\n");
                return END_EXECUTION;
            }
            else if(errcode >= 150)
            {
                fprintf(stderr, "error in ADVERTISE TCP message creation. The buffer was not big enough\n");
                return LEAVE_NETWORK;
            }
            // enviar o ADVERTISE
            if (writeTCP(external->fd, strlen(message_buffer), message_buffer) == ERROR)
                errcode = ERROR;
        }
        else
            errcode = ERROR;
    }
    // neste caso houve um erro ao tentar ligar ao nó escolhido aleatoriamente, ou a fazer write
    if (errcode == ERROR)
    {
        // segundo a manpage do connect, se falhar o connect temos de dar close da socket
        // caso falhe algum dos write também vamos fechar a socket
        if (close(external->fd) == -1)
            fprintf(stderr, "Error closing file descriptor for unavailable chosen external neighbour: %s\n", strerror(errno));
        return ERROR;
    }
    return NO_ERROR;
}

int chooseExternalFromList(node_list *list_of_nodes, int num_nodes, viz *external, no *self, char *message_buffer)
{
    // escolher um nó da lista pseudo-aleatório 
    node_list *aux_list_of_nodes = list_of_nodes;
    int random_index = rand() % num_nodes, errcode;
    node_list *prev_list_of_nodes = NULL;
    for (int i = 0; i < random_index; i++)
    {
        prev_list_of_nodes = aux_list_of_nodes;
        aux_list_of_nodes = aux_list_of_nodes->next;
    }
    // tentar ligar ao nó selecionado
    errcode = setListedNodeAsExternal(external, aux_list_of_nodes, self, message_buffer);

    // neste caso houve um erro que não necessita o encerro do programa ao tentar ligar ao nó escolhido aleatoriamente, ou a fazer write
    if (errcode == ERROR)
    {
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
        // tentamos todos os nós recebidos até ou nos ligarmos a um ou sabermos que todos os nós que lá estavam
        // não estão a aceitar ligações por TCP
        while (aux_list_of_nodes)
        {
            errcode = setListedNodeAsExternal(external, aux_list_of_nodes, self, message_buffer);
            // neste caso o errcode será NO_ERROR se tanto o connect como ambos os writes funcionaram
            // já estamos ligados ao nosso vizinho externo e já lhe enviámos o ADVERTISE e o NEW
            if (errcode == NO_ERROR || errcode == END_EXECUTION)
                break;
            // continuamos a iterar pela lista, de modo a tentarmos outros nós listados
            aux_list_of_nodes = aux_list_of_nodes->next;
        }
    }
    // neste caso ligámo-nos a algum nó e colocámo-lo como vizinho externo
    if (errcode == NO_ERROR)
    {
        strncpy((external)->IP, aux_list_of_nodes->IP, NI_MAXHOST);
        strncpy((external)->port, aux_list_of_nodes->port, NI_MAXSERV);
        return NO_ERROR;
    }
    return errcode;
}

int setupExternal(char *list_msg, viz **external, no *self)
{
    node_list *list_of_nodes = NULL;
    int num_nodes = 0, errcode;
    char message_buffer[150];
    // criar uma lista simplesmente ligada com os contactos dos nós listados (essa lista é list_of_nodes)
    // caso haja um erro, devolvemos esse erro à main e limpamos a memória alocada durante a parseNodeListRecursive
    if((errcode = parseNodeListRecursive(list_msg, &list_of_nodes, &num_nodes)) == END_EXECUTION || errcode == LEAVE_NETWORK)
    {
        freeNodeList(&list_of_nodes);
        return errcode;
    }
    *external = malloc(sizeof(viz));
    if(*external == NULL)
    {
        freeNodeList(&list_of_nodes);
        fprintf(stderr, "Malloc error allocating memory for external neighbou\n");
        return END_EXECUTION;
    }
    (*external)->next_av_ix = 0;
    // preparar mensagem new, com a informação do IP/porto do nosso servidor TCP 
    errcode = snprintf(message_buffer, 150, "NEW %s %s\n", self->IP, self->port);  
    if (errcode < 0)
    {
        freeNodeList(&list_of_nodes);
        fprintf(stderr, "error in NEW TCP message creation: snprintf returned a negative value\n");
        return END_EXECUTION;
    }
    else if(errcode >= 150)
    {
        freeNodeList(&list_of_nodes);
        fprintf(stderr, "error in NEW TCP message creation: snprintf wanted to write more characters than the buffer allowed. Consider increasing the buffer's size\n");
        return LEAVE_NETWORK;
    }
    errcode = ERROR;
    // se a lista recebida estava bem formatada (já havia nós na rede selecionada)
    if (list_of_nodes)
        errcode = chooseExternalFromList(list_of_nodes, num_nodes, *external, self, message_buffer);
    // neste caso não nos conseguimos ligar a nenhum dos nós listados no servidor de nomes
    // vamos entrar na rede como nó único, assumindo que os outros falharam de algum modo
    // ou estão no processo
    // de enviar o UNREG
    if (errcode == ERROR)
    {
        free(*external);
        *external = NULL;
    }  
    // clear list
    freeNodeList(&list_of_nodes);
    return errcode;
}

int reg(no *self, char *send_IP, char *send_UDP)
{
    char dgram[1000];
    // criar string para enviar o registo do nó
    int errcode = snprintf(dgram, 1000, "REG %s %s %s", self->net, self->IP, self->port);  
    if (errcode < 0)
    {
        fprintf(stderr, "error in REG UDP message creation. snprintf returned a negative value\n");
        return END_EXECUTION;
    }
    else if(errcode >= 150)
    {
        fprintf(stderr, "error in REG UDP message creation. snprintf wanted to write past the buffer. Consider increasing its size\n");
        return LEAVE_NETWORK;
    }

    errcode = getDgram("error in REG UDP message send\n", send_IP, send_UDP, dgram);
    if (errcode == DGRAM_RECEIVED)
    {
        if (!strcmp(dgram, "OKREG"))
            printf("We received the confirmation of registration from the server\n");
        else
            fprintf(stderr, "We received gibberish via UDP\n");
        return NO_ERROR;
    }
    if (errcode == NON_FATAL_ERROR)
    {
        printf("There was a problem registering in the server. We will proceed execution as normal\n");
        return NO_ERROR; // coloco NO_ERROR apesar de ter havido um erro porque para a função main este erro é irrelevante. Notificamos o utilizador
        // do problema com o servidor e seguimos tudo normalmente
    }
    return errcode; // neste caso ERRCODE será END_EXECUTION
}
/*
node_list *parseNodelist(char* datagram, int *num_nodes)
{
    int datagram_ix = 0, line_ix=0;
    node_list* list = NULL;
    *num_nodes = -1;
    char c = datagram[datagram_ix];
    char line[150]; // colocar aqui uma variavel com #DEFINE 150
    while (c != '\0')
    {
        // quando num_nodes == -1 estamos a ler o comando e nao um par IP/porto
        if (*num_nodes != -1)
        {
            line[line_ix] = c;
            line_ix++;
        }
        if (c == '\n')
        {
            (*num_nodes)++;
            // neste caso ou é zero, e portanto lemos apenas o comando
            // ou devemos fazer um elemento da lista de nós
            if (*num_nodes)
            {   
                line[line_ix] = '\0';
                addLineToList(&list, line);
                line_ix = 0; // reiniciar o indice, porque vamos começar a ler uma linha nova
            }
        }
        datagram_ix++;
        c = datagram[datagram_ix];
    }
    return list;
}

void addLineToList(node_list **list, char *line)
{
    node_list *this = safeMalloc(sizeof(node_list));
    this.next = NULL;
    node_list *aux = *list;
    sscanf(line,"%s %s ", this.IP, this.port);
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
*/

