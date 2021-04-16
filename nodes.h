#ifndef NODES_H
#define NODES_H

#include <netdb.h>


#define N_MAX 150
typedef struct no{
    char *net; //identificador da rede
    char *id;    //identificador do nó
    char IP[NI_MAXHOST]; //endereço IP do nó
    char port[NI_MAXSERV]; //Porto TCP do nó
}no;

typedef struct viz{
    int fd; //file descriptor associado à conexão com o vizinho 
    char IP[NI_MAXHOST]; //endereço IP do vizinho
    char port[NI_MAXSERV]; //Porto TCP do vizinho
    char buffer[N_MAX+1]; // buffer que guarda os caracteres da stream
    // proximo indice de buffer onde podemos colocar um caractere. Fica com 
    // este tipo (ssize_t) porque entra para os calculos do argumento do read
    ssize_t next_av_ix; 
}viz;

// lista dos vizinhos internos do nó
typedef struct internals{
    struct viz *this;
    struct internals *next;
}internals;

void addToList(internals **int_neighbours, viz *new);
#endif
