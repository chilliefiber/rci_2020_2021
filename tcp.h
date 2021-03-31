#ifndef TCP_H_CONST
#define TCP_H_CONST
#define MSG_PARTIAL 0 // indica que recebemos via TCP texto mas que não recebemos nenhum \n
#define MSG_FINISH 1 // indica que recebemos via TCP texto e que recebemos pelo menos 1 \n
#define MSG_READ_ERROR 2 // indica que houve um erro no read
#define MSG_CLOSED 3 // indica que o read() devolveu 0, logo fecharam a conexão ordeiramente
#define MSG_FORMAT_ERROR 4 // indica que o tipo que nos está a enviar texto excedeu o tamanho limite de uma mensagem sem enviar o \n
#define N_MAX 150

// estrutura para criar uma lista das diferentes 
// mensagens lidas numa chamada de sistema read
typedef struct messages{
  char *message;
  struct messages* next;
} messages; 

typedef struct viz{
    int flag_interest;
    int fd; //file descriptor associado à conexão com o vizinho 
    char IP[NI_MAXHOST]; //endereço IP do vizinho
    char port[NI_MAXSERV]; //Porto TCP do vizinho
    char buffer[N_MAX+1]; // buffer que guarda os caracteres da stream
    // proximo indice de buffer onde podemos colocar um caractere. Fica com 
    // este tipo (ssize_t) porque entra para os calculos do argumento do read
    ssize_t next_av_ix; 
}viz;
/**
 * writeTCP faz o write da ligação TCP
 * \param fd file descriptor para onde a ligação vai ser feita
 * \param nleft número de bytes que é para enviar
 * \param buffer texto que é para ser enviado
 * \return 1 caso envio seja bem sucedido, 0 caso contrário
 */
char writeTCP(int fd, ssize_t nleft, char *buffer); 

/**
 * messagesAlloc faz a alocação da estrutura messages
 * \return estrutura messages alocada
 */
messages* messagesAlloc(void);

messages *processReadTCP(viz *sender, ssize_t start_ix);


char readTCP(viz* sender);
void freeMessage(messages *msg);
#endif 
