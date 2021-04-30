#ifndef NODES_H
#define NODES_H

#include <netdb.h>

#define N_MAX 150
// estrutura que contem informação do nó da aplicação a correr
typedef struct no{
    char *net; //identificador da rede
    char *id;    //identificador do nó
    char IP[NI_MAXHOST]; //endereço IP do nó
    char port[NI_MAXSERV]; //Porto TCP do nó
    int server_fd; // fd associado ao servidor TCP
}no;

// estrutura que contem informação dum vizinho
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

/**
 * addtoList: função que adiciona um interno à lista de vizinhos internos (inserção pelo topo da lista)
 * \param **int_neighbours: duplo ponteiro para o topo da lista de vizinhos internos
 * \param *new: ponteiro para uma variável da estrutura viz, que conterá a informação do novo vizinho que se vai adicionar à lista
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int addToList(internals **int_neighbours, viz *new);

/**
 * freeViz: função que liberta a memória alocada por uma variável do tipo viz
 * \param **viz: duplo ponteiro para a variável do tipo viz
 */
void freeViz(viz **v);

/**
 * freeIntNeighbours: função que liberta a memória alocada na lista de vizinhos internos
 * \param **int_neighbours: duplo ponteiro para o topo da lista de vizinhos internos
 */
void freeIntNeighbours(internals **int_neighbours);

/**
 * freeSelf: função que liberta a memória alocada no nó da aplicação
 * \param *self: ponteiro para a variável da estrutura no
 */
void freeSelf(no *self);
#endif
