#ifndef TCP_H_CONST
#define TCP_H_CONST
#define MSG_PARTIAL 0 // indica que recebemos via TCP texto mas que não recebemos nenhum \n
#define MSG_FINISH 1 // indica que recebemos via TCP texto e que recebemos pelo menos 1 \n
#define MSG_READ_ERROR 2 // indica que houve um erro no read
#define MSG_CLOSED 3 // indica que o read() devolveu 0, logo fecharam a conexão ordeiramente
#define MSG_FORMAT_ERROR 4 // indica que o tipo que nos está a enviar texto excedeu o tamanho limite de uma mensagem sem enviar o \n

#include "nodes.h"
// estrutura para criar uma lista das diferentes 
// mensagens lidas numa chamada de sistema read
typedef struct messages{
  char *message;
  struct messages* next;
} messages; 

/**
 * writeTCP faz o write da ligação TCP dentro dum ciclo
 * \param fd file descriptor para onde fazemos write
 * \param nleft número de bytes que é para enviar
 * \param buffer texto que é para ser enviado
 * \return NO_ERROR caso envio seja bem sucedido, ERROR caso contrário
 */
int writeTCP(int fd, ssize_t nleft, char *buffer); 

/**
 * messagesAlloc faz a alocação da estrutura messages
 * \return estrutura messages alocada
 */
messages* messagesAlloc(void);

/**
 * processReadTCP cria uma lista simplesmente ligada com todas as mensagens 
 * armazenadas na buffer do sender 
 * \param sender vizinho associado à buffer
 * \param start_ix indíce de início da mensagem a ser processada nesta chamada da função
 * \param errcode flag que indica se houve algum erro durante a execução do programa
 * \return lista com as mensagens enviadas por sender
 */
messages *processReadTCP(viz *sender, ssize_t start_ix, int *errcode);

/**
 * readTCP lê uma vez do fd associado ao sender, e armazena os bytes lidos na buffer do sender
 * \param sender vizinho que enviou bytes
 * \return código indicativo do que aconteceu na função: MSG_READ_ERROR, MSG_CLOSED, MSG_FORMAT_ERROR, MSG_FINISH, MSG_PARTIAL
 */
char readTCP(viz* sender);
/**
 * freeMessage Limpa uma estrutura alocada pelo messagesAlloc
 */
void freeMessage(messages *msg);

/**
 * freeMessageListLimpa uma lista de estruturas alocadas pelo messagesAlloc
 */
void freeMessageList(messages **msg_list);
#endif 
