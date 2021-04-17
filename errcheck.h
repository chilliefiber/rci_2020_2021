#ifndef ERROR_CHECKING_H
#define ERROR_CHECKING_H

#define END_EXECUTION 0
#define LEAVE_NETWORK 1
#define NO_ERROR 2
#define NON_FATAL_ERROR 3
#define ERROR 4 // erro genérico, não caracterizamos como fatal ou não

#include <stddef.h>
#include <netdb.h>

void* safeMalloc(size_t size);

/**
 * \param fd: fd do client UDP
 * \param ip: ip do servidor UDP
 * \param port: porto do cliente UDP
 * \param text: texto que queremos enviar no datagrama
 * \param addrinfo_error_msg: mensagem de erro para o utilizador na chamada ao getaddrinfo
 * \return ERROR se houve um erro no processo de envio, NO_ERROR caso contrário
 */
int sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg);
/**
 * \param ip: endereço ao qual nos queremos ligar
 * \param port: porto ao qual nos queremos ligar
 * \param hints: variável auxiliar necessária 
 * \param res: variável auxiliar necessária
 * \param error_msg: mensagem de erro a apresentar ao utilizador
 * \return 0 em caso de algum erro, 1 caso tudo funcione
 */
int safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg);
void* safeCalloc(size_t nmemb, size_t size);
void safeRecvFrom(int fd, char *dgram, size_t len);
int safeTCPSocket(int* fd);

/**
 * \param ip: endereço ao qual nos queremos ligar
 * \param port: porto ao qual nos queremos ligar
 * \param fd: fd que queremos que fique com a ligação
 * \param addrinfo_error_msg: mensagem de erro na chamada ao getaddrinfo
 * \param connect_error_msg: mensagem de erro na chamada ao connect
 * \return 0 em caso de algum erro, 1 caso tudo funcione
 */
int connectTCP(char *ip, char* port, int fd, char *addrinfo_error_msg, char *connect_error_msg);
#endif
