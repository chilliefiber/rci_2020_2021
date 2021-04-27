#ifndef ROUTING_H
#define ROUTING_H

// tabela de expedição 
typedef struct tab_entry{
    char *id_dest;
    int fd_sock;
    struct tab_entry *next;
}tab_entry;

/**
 * createinsertTabEntry: função que cria uma entrada na tabela de expedição (basicamente cria e adiciona um nó a lista simplesmente ligada) e retorna o topo da lista atualizado no final
 * \param **first_entry: duplo ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 * \param *id_dst: ponteiro para a string correspondente ao identificador do nó destino correspondente à entrada da tabela que se vai adicionar
 * \param fd: inteiro correspondente ao file descriptor que está associado à sessão TCP que nos permite chegar ào nó destino 
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int createinsertTabEntry(tab_entry **first_entry, char *id_dst, int fd);

/**
 * deleteTabEntryid: função que recebe como argumento um id e que elimina a respetiva entrada da tabela de expedição (basicamente deleta um nó da lista simplesmente ligada) 
 * \param **first_entry: duplo ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 * \param *id_out: ponteiro para a string correspondente ao identificador do nó cuja entrada correspondente vai ser eliminada
 */
void deleteTabEntryid(tab_entry **first_entry, char *id_out);

/**
 * deleteTabEntryfd: função que recebe como argumento um fd e que elimina a respetiva entrada da tabela de expedição (basicamente deleta um nó da lista simplesmente ligada) 
 * \param **first_entry: duplo ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 * \param fd_out: inteiro correspondente ao file descriptor que está associado à sessão TCP que nos permitia chegar ào nó destino, a entrada correspondente vai ser eliminada
 */
void deleteTabEntryfd(tab_entry **first_entry, int fd_out);

/** 
 * checkTabEntry: função que verifica se existe na tabela de expedição uma entrada com o identificador id (importante para pesquisa de objetos nomeados)
 * \param *first_entry: ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 * \param *id: ponteiro para a string correspondente ao identificador do nó
 * \retorna 1 se existir a entrada na tabela e retorna 0 caso não exista
 */
int checkTabEntry(tab_entry *first_entry, char *id);

/** 
 * printTabExp: função que imprime a tabela de expedição do nó da aplicação
 * \param *first_entry: ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 */
void printTabExp(tab_entry *first_entry);

/** 
 * FreeTabExp: função que liberta a tabela de expedição (desaloca a memória alocada pelas entradas)
 * \param *first_entry: duplo ponteiro para a primeira entrada da tabela de expedição (topo da lista)
 */
void FreeTabExp(tab_entry **first_entry);
#endif
