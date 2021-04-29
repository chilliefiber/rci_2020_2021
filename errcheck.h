#ifndef ERROR_CHECKING_H
#define ERROR_CHECKING_H

#define END_EXECUTION 0
#define LEAVE_NETWORK 1
#define NO_ERROR 2
#define NON_FATAL_ERROR 3
#define ERROR 4 // erro genérico, não caracterizamos como fatal ou não

#include <stddef.h>
#include <netdb.h>

/**
 * sendUDP envia um datagrama por UDP
 * \param fd descritor do nosso cliente UDP
 * \param ip endereço para onde vamos enviar o datagrama
 * \param port porto para onde vamos o datagrama
 * \param text texto que vamos enviar por UDP
 * \param addrinfo_error_msg mensagem de erro a apresentar ao utilizador em falha da busca DNS
 * \param send_error_msg mensagem de erro a apresentar ao utilizador em falha no envio
 * \return ERROR se houve um erro, NO_ERROR caso contrário
 */
int sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg);
/**
 * \param ip: endereço ao qual nos queremos ligar
 * \param port: porto ao qual nos queremos ligar
 * \param hints: variável auxiliar necessária 
 * \param res: variável auxiliar necessária
 * \param error_msg: mensagem de erro a apresentar ao utilizador
 * \return ERROR em caso de algum erro, NO_ERROR caso tudo funcione
 */
int safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg);

/**
 * safeRecvFrom recebe um datagrama por UDP
 * \param fd descritor do nosso cliente UDP
 * \param dgram array de caracteres onde devemos guardar o datagrama
 * \param len tamanho de dgram menos um
 * \return END_EXECUTION caso o recvfrom encontre um erro, NO_ERROR caso contrário
 */
int safeRecvFrom(int fd, char *dgram, size_t len);

/**
 * safeTCPSocket cria uma socket para um cliente TCP
 * \param fd descritor que vai ficar associado à socket
 * \return ERROR se a função socket() encontrou um erro, NO_ERROR caso contrário
 */
int safeTCPSocket(int* fd);

/**
 * \param ip: endereço ao qual nos queremos ligar
 * \param port: porto ao qual nos queremos ligar
 * \param fd: fd que queremos que fique com a ligação
 * \param addrinfo_error_msg: mensagem de erro na chamada ao getaddrinfo
 * \param connect_error_msg: mensagem de erro na chamada ao connect
 * \return ERROR em caso de algum erro, NO_ERROR caso tudo funcione
 */
int connectTCP(char *ip, char* port, int fd, char *addrinfo_error_msg, char *connect_error_msg);
#endif
