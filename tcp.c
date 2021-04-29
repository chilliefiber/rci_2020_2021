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
int writeTCP(int fd, ssize_t nleft, char *buffer)
{
    ssize_t nwritten;

    while(nleft>0){
        nwritten = write(fd, buffer, nleft);
        if (nwritten <= 0)
            return ERROR;
        nleft -= nwritten;
        buffer += nwritten;
    }
    return NO_ERROR;
}

messages* messagesAlloc(void)
{
  messages* new = malloc(sizeof(messages));
  if (!new)
      return NULL;
  new->message = calloc(N_MAX+1, sizeof(char));
  if (!new->message)
      return NULL;
  new->next = NULL;
  return new;
}

messages *processReadTCP(viz *sender, ssize_t start_ix, int *errcode)
{
    // este é o caso em que a função é chamada pelo main
    messages* new = messagesAlloc();
    if(new == NULL)
    {
        *errcode = END_EXECUTION;
        return NULL;
    }

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
                new->next = processReadTCP(sender, ix + 1, errcode);
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
    }
    else if (n_read == -1)
        return MSG_READ_ERROR;
    else if (!n_read)
        return MSG_CLOSED;
    // se já lemos N_MAX caracteres e ainda não
    // recebemos uma \n, está a ocorrer um erro
    // da parte do sender
    if (sender->next_av_ix == N_MAX && !message_end)
        return MSG_FORMAT_ERROR;
    if (message_end)
        return MSG_FINISH;
    return MSG_PARTIAL;
}

void freeMessage(messages *msg)
{
    free(msg->message);
    free(msg);
}

void freeMessageList(messages **msg_list)
{
    messages *msg_aux;
    while (*msg_list != NULL)
    {
        msg_aux = *msg_list;
        *msg_list = (*msg_list)->next;
        freeMessage(msg_aux);
    }
}
