// estrutura para criar uma lista das diferentes 
// mensagens lidas numa chamada de sistema read
typedef struct messages{
  char *message;
  struct messages* next;
} messages; 

typedef struct viz{
    int fd; //file descriptor associado à conexão com o vizinho 
    char IP[INET_ADDRSTRLEN]; //endereço IP do vizinho
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


typedef struct stream{
  int fd; // file descriptor da stream
  char buffer[N_MAX+1]; // buffer que guarda os caracteres da stream
  // proximo indice de buffer onde podemos colocar um caractere. Fica com 
  // este tipo porque entra para os calculos do argumento do read
  ssize_t next_av_ix; 
} stream;
 
