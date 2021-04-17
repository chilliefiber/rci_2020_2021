#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
// #include <netdb.h> esta foi incluída no header file porque 
// dava erro ao utilizar funções do tcp.c em ficheiros que não o main
// caso não estivesse lá, devido à declaração do NI_MAXHOST/NI_MAXSERV
#include "tcp.h"
#include "errcheck.h"
#include <stdio.h> // depois tirar este
char writeTCP(int fd, ssize_t nleft, char *buffer)
{
    fputs( "We're writing shit through TCP mudafucka\n",stdout);
    ssize_t nwritten;

    while(nleft>0){
        nwritten = write(fd, buffer, nleft);
        if (nwritten <= 0)
            return 0;
        nleft -= nwritten;
        buffer += nwritten;
    }
    return 1;
}


messages* messagesAlloc(void)
{
  messages* new = safeMalloc(sizeof(messages));
  if (!new)
      return NULL;
  new->message = safeCalloc(N_MAX+1, sizeof(char));
  if (!new->message)
      return NULL;
  new->next = NULL;
  return new;
}

messages *processReadTCP(viz *sender, ssize_t start_ix)
{
    // este é o caso em que a função é chamada pelo main
    messages* new = messagesAlloc();
    char message_end = 0;
    ssize_t ix;
    for (ix = start_ix; ix < sender->next_av_ix; ix++)
    {
        // se encontrámos o fim de uma mensagem
        if (sender->buffer[ix] == '\n'){
            message_end = 1;
            // começar a mensagem em start_ix e acabar em ix, inclusive, daí o +1
            strncpy(new->message, sender->buffer + start_ix, ix - start_ix + 1);
            // como só estamos a ler até ao \n colocamos já o \0 a seguir ao \n
            new->message[ix - start_ix + 1] = '\0';
            // se ainda há mais texto para lá do desta mensagem
            if (ix + 1 != sender->next_av_ix)
                // começar a proxima mensagem a ler a partir do proximo indice
                new->next = processReadTCP(sender, ix + 1);
            // se estávamos no ultimo indice disponivel
            // nao alocamos outra estrutura, nem limpamos memória
            // reinicializamos apenas o indice da buffer
            else
                sender->next_av_ix = 0;
            break;
        }
    }
    // se ficou uma mensagem parcialmente recebida, queremos colocar
    // o texto que sobrou no inicio da buffer da stream
    if (!message_end)
    {
        strncpy(sender->buffer, sender->buffer + start_ix, ix - start_ix);
        sender->next_av_ix = ix - start_ix;
        freeMessage(new);
        new = NULL;
    }
    return new;
}

char readTCP(viz* sender)
{
    printf("Estamos a ler por TCP\n");
    ssize_t n_read, i;
    char  message_end=0;
    // N_MAX é o limite superior de uma mensagem
    // sender->next_av_ix é o proximo indice disponivel
    // da buffer desta stream
    // nao queremos escrever para la da stream, daí a subtração
    //
    // ter em atenção quando tiver menos sono, ver se há a possibilidade de N_MAX == sender->next_av_ix
    // é que nesse caso o comportamento do read é esquisito. Possivelmente incluir uma verificação aqui
    if ((n_read=read(sender->fd, sender->buffer + sender->next_av_ix, N_MAX - sender->next_av_ix)) > 0){
        // verificamos se nos caracteres recebidos não estava um \n.
        // se estava, era o fim da mensagem se bem formatada.
        for (i=0; i < n_read; i++){
            if (sender->buffer[i+sender->next_av_ix] == '\n'){
                message_end=1;
                break;
            }
        }
        // adicionamos o valor de caracteres lidos agora ao lido no total
        sender->next_av_ix += n_read;
        printf("Lemos coisas por TCP\n");
    }
    else if (n_read == -1)
    {
        printf("Houve um erro na leitura\n");
        return MSG_READ_ERROR;
    }
    else if (!n_read)
    {
        printf("O tipo fechou a conexão\n");
        return MSG_CLOSED;
    }
    // se já lemos N_MAX caracteres e ainda não
    // recebemos uma \n, está a ocorrer um erro
    // da parte do sender
    if (sender->next_av_ix == N_MAX && !message_end)
    {
        printf("O gajo enviou dados a mais\n");
        return MSG_FORMAT_ERROR;
    }
    if (message_end){
        printf("Recebemos uma mensagem completa no readTCP\n");
        return MSG_FINISH;
    }
    printf("Recebemos uma mensagem parcial por TCP\n");
    return MSG_PARTIAL;
}

void freeMessage(messages *msg)
{
    free(msg->message);
    free(msg);
}
