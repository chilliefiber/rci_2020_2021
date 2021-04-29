#ifndef UDP_PARSER_H
#define UDP_PARSER_H

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include "nodes.h"

typedef struct node_list{
    char IP[NI_MAXHOST]; //endereço IP do nó	
    char port[NI_MAXSERV]; //Porto TCP do nó
    struct node_list *next;
} node_list;
/**
 * parseNodeListRecursive cria uma lista com todos os contactos de todos os nós listados em datagram
 * \param dgram datagrama da NODESLIST, saltando a primeira linha (ou seja, começa no início da primeira linha
 * com contactos de nós)
 * \param list lista que vai ser criada
 * \param num_nodes guarda o número de nós listados
 *\return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK e NO_ERROR
 */
int parseNodeListRecursive(char* datagram, node_list **list, int *num_nodes);


/**
 * isNodesList verifica se a primeira linha do datagram está bem formatada
 * \param datagram datagrama NODESLIST
 * \param net identificador da rede para a qual pedimos a lista de nós
 * \param nodeslist_received flag que indica se a primeira linha do datagrama estava bem formatada
 * \param error_flag: indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK e NO_ERROR
 * \return início da segunda linha do datagrama NODESLIST, ou NULL se houve um erro ou era apenas uma linha
 */
char* isNodesList(char* datagram, char *net, char *nodeslist_received, int *error_flag);

/**
 * freeNodeList limpa a memória de uma lista de contactos
 * \param list lista de contactos
 */ 
void freeNodeList(node_list **list);

/**
 * sendAndWait faz o envio de uma mensagem por UDP, e select apenas com o fd associado ao cliente UDP com timeout
 * \param fd_udp fd associado ao client UDP
 * \param tv estrutura com a informação do tempo para esperar
 * \param send_error_msg mensagem de erro para o utilizador numa falha do send
 * \param send_IP IP do servidor UDP
 * \param send_UDP porto do servidor UDP
 * \param dgram datagrama a enviar
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, TIMER_EXPIRED, OK_READ 
 */
int sendAndWait(int fd_udp, struct timeval *tv, char* send_error_msg, char *send_IP, char *send_UDP, char *dgram);

/**
 * getDgram implementa um sistema de várias retransmissões por UDP em caso de timeout no envio, e recebe a resposta (se possível) e coloca-a em dgram 
 * \param send_error_msg mensagem de erro para o utilizador numa falha do send
 * \param send_IP IP do servidor UDP
 * \param send_UDP porto do servidor UDP
 * \dgram datagrama que é para enviar. No final dgram tem a mensagem recebida se houve alguma
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, DGRAM_RECEIVED
 */
int getDgram(char *send_error_msg, char *send_IP, char *send_UDP, char *dgram);

/**
 * unreg Apaga o registo no servidor
 * \param self estrutura com a informação da nossa rede, do nosso IP e porto
 * \param send_IP IP do servidor UDP
 * \param send_UDP porto do servidor UDP
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int unreg(no *self, char *send_IP, char *send_UDP);

/**
 * getNodesList envia a mensagem NODES para pedir a lista de contactos na rede
 * \param self_net rede em questão
 * \param send_IP IP do servidor UDP
 * \param send_UDP porto do servidor UDP
 * \param datagrama recebido após o envio do NODES
 * \return inteiro indicativo de um código de erro: NON_FATAL_ERROR, END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int getNodesList(char *self_net, char *send_IP, char *send_UDP, char *dgram);

/**
 * setListedNodeAsExternal tenta estabelecer um nó anunciado pelo servidor como vizinho externo. Faz connect
 * e write do NEW e do ADVERTISE
 * \param external estrutura com a informação do nosso vizinho externo
 * \param aux_list_of_nodes estrutura com a informação do nó anunciado
 * \param self estrutra com a informação da nossa rede, do nosso IP e porto
 * \param message_buffer string com a mensagem NEW que vamos tentar enviar
 * \return inteiro indicativo de um código de erro: ERROR, END_EXECUTION, NO_ERROR
 */
int setListedNodeAsExternal(viz *external, node_list *aux_list_of_nodes, no *self, char *message_buffer);

/**
 * chooseExternalFromList seleciona um nó anunciado pelo servidor aleatoriamente e tenta colocá-lo como externo
 * Caso não consiga, tenta todos os outros que foram anunciados
 * \param list_of_nodes lista de nós anunciados
 * \param num_nodes nº de nós anunciados
 * \param external estrutura com a informação do nosso vizinho externo
 * \param self estrutura com a informação da nossa rede, do nosso IP e porto
 * \param message_buffer string com a mensagem NEW que vamos tentar enviar
 * \return inteiro indicativo de um código de erro: ERROR, END_EXECUTION, NO_ERROR
 */
int chooseExternalFromList(node_list *list_of_nodes, int num_nodes, viz *external, no *self, char *message_buffer);

/**
 * setupExternal cria e inicializa a estrutura para o vizinho externo, prepara a mensagem NEW e chama a chooseExternalFromList
 * \param list_msg parte da mensagem NODESLIST que tem os contactos dos vizinhos anunciados
 * \param external estrutura para o vizinho externo
 * \param self estrutura com a informação da nossa rede, do nosso IP e porto
 * \return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK, ERROR, NO_ERROR  
 */
int setupExternal(char *list_msg, viz **external, no *self);

/**
 * reg Faz o nosso registo no servidor UDP
 * \param self estrutura com a informação da nossa rede, do nosso IP e porto
 * \param send_IP IP do servidor UDP
 * \param send_UDP porto do servidor UDP
 * \return inteiro indicativo de um código de erro: END_EXECUTION, LEAVE_NETWORK, NO_ERROR
 */
int reg(no *self, char *send_IP, char *send_UDP);

#endif
