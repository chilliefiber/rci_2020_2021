// estrutura para criar uma lista das diferentes 
// mensagens lidas numa chamada de sistema read
typedef struct messages{
  char *message;
  struct messages* next;
} messages; 

/**
 * writeTCP faz o write da ligação TCP
 * \param fd file descriptor para onde a ligação vai ser feita
 * \param nleft número de bytes que é para enviar
 * \param buffer texto que é para ser enviado
 * \return 1 caso envio seja bem sucedido, 0 caso contrário
 */
char writeTCP(int fd, ssize_t nleft, char *buffer); 

typedef struct stream{
  int fd; // file descriptor da stream
  char buffer[N_MAX+1]; // buffer que guarda os caracteres da stream
  // proximo indice de buffer onde podemos colocar um caractere. Fica com 
  // este tipo porque entra para os calculos do argumento do read
  ssize_t next_av_ix; 
} stream;
 
