#ifndef SEARCH_H
#define SEARCH_H

// estrutura correspondente a uma lista auxiliar para cache (caso WITHDRAW retirar elementos com o id)
typedef struct cache_aux{
    char *obj;
    struct cache_aux *next;
}cache_aux;

// lista de objetos nomeados do nó
typedef struct list_objects{
    char *objct;
    struct list_objects *next;
}list_objects;

// lista de pedidos (interesse de vizinhos)
typedef struct list_interest{
    int fd;
    char *obj;
    struct list_interest *next;
}list_interest;

/**
 * addInterest: função que adiciona um pedido de interesse à lista de pedidos
 * \param **first_interest: duplo ponteiro para o primeiro pedido da lista (topo da lista)
 * \param *obj: ponteiro para a string correspondente ao nome do objeto que foi pedido
 * \param fd: inteiro correspondente ao file descriptor que está associado à sessão TCP donde veio a mensagem de interesse
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int addInterest(list_interest **first_interest, char *obj, int fd);

/**
 * deleteInterest: função que elimina um pedido de interesse da lista de pedidos
 * \param **first_interest: duplo ponteiro para o primeiro pedido da lista (topo da lista)
 * \param *obj: ponteiro para a string correspondente ao nome do objeto presente no pedido a eliminar
 * \param fd: inteiro correspondente ao file descriptor que está associado à sessão TCP donde veio a mensagem de interesse
 */
void deleteInterest(list_interest **first_interest, char *obj, int fd);

/**
 * deleteInterestfd: função que elimina um pedido de interesse da lista de pedidos (usada nos casos em que perdemos conexão com vizinhos, para eliminar possíveis pedidos que tenham vindo desse lado)
 * \param **first_interest: duplo ponteiro para o primeiro pedido da lista (topo da lista)
 * \param fd: inteiro correspondente ao file descriptor que está associado à sessão TCP donde veio a mensagem de interesse
 */
void deleteInterestfd(list_interest **first_interest, int fd);

/**
 * deleteInterestWITHDRAW: função que elimina da lista de pedidos todos os pedidos para um objeto que tivesse identificador id (usada quando recebemos mensagem WITHDRAW)
 * \param **first_interest: duplo ponteiro para o primeiro pedido da lista (topo da lista)
 * \param *id: inteiro correspondente ao identificador do nó que saiu da rede (que recebemos na mensagem WITHDRAW)
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int deleteInterestWITHDRAW(list_interest **first_interest, char *id);

/**
 * checkInterest: função que verifica se um determinado pedido é igual a um que já exista na lista de pedidos (pedido igual vindo do mesmo vizinho)
 * \param *first_interest: ponteiro para o primeiro pedido da lista (topo da lista)
 * \param *obj: ponteiro para a string correspondente ao nome do objeto que foi pedido
 * \param fd: inteiro correspondente ao file descriptor que está associado à sessão TCP donde veio a mensagem de interesse
 * \retorna 1 caso o mesmo pedido tenha vindo do mesmo vizinho, 0 caso contrário
 */
int checkInterest(list_interest *first_interest, char *obj, int fd);

/**
 * FreeInterestList: função que liberta a memória alocada na lista de pedidos de interesse
 * \param **first_interest: duplo ponteiro para o primeiro pedido da lista (topo da lista)
 */
void FreeInterestList(list_interest **first_interest);

/**
 * getidfromName: função que extrai do nome introduzido pelo comando "get" o ientificador do nó destino para a pesquisa do objeto
 * \param *name: ponteiro para a string correspondente ao nome introduzido pelo comando "get"
 * \param *id: ponteiro para a string correspondente ao identificador do nó que será retornado no final desta função
 * \retorna um ponteiro para a string correspondente ao identificador do nó 
 */
char *getidfromName(char *name, char *id);

/** 
 * getConcatString: função que concatena duas strings numa só (utilizada na criação de objetos para juntar id ao subnome)
 * \param *str1: ponteiro para a primeira string (corresponderá ao identificador do nó)
 * \param *str2: ponteiro para a segunda string (corresponderá ao subnome do objeto)
 * \retorna um ponteiro para a string resultante
 */
char *getConcatString( const char *str1, const char *str2);

/** checkObjectList: função que verifica se existe um determinado objeto na lista de objetos nomeados do nó (importante para pesquisa de objetos nomeados)
 * \param *head_obj: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 * \param *name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \retorna 1 se existir o objeto na lista e retorna 0 caso não exista
 */
int checkObjectList(list_objects *head_obj, char *name);

/**
 * createinsertObject: função que cria um objeto e adiciona a lista de objetos nomeados do nó (basicamente cria e adiciona um nó a lista simplesmente ligada) e retorna o topo da lista atualizado no final
 * \param *head: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 * \param *subname: ponteiro para a string correspondente ao subnome do objeto introduzido no comando "create"
 * \param *id: ponteiro para a string correspondente ao identificador do nó da aplicação 
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int createinsertObject(list_objects **head, char *subname, char *id);

/** printTabExp: função que imprime a lista de objetos nomeados do nó da aplicação
 * \param *head_obj: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 */
void printObjectList(list_objects *head_obj);

/** FreeObjectList: função que liberta a lista de objetos nomeados (desaloca a memória alocada pelos objetos(nós) da lista)
 * \param **head_obj: duplo ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 */
void FreeObjectList(list_objects **head_obj);

/** checkCache: função que verifica se existe um determinado objeto na cache do nó (importante para pesquisa de objetos nomeados)
 * \param **cache: duplo ponteiro para a cache
 * \param *name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 * \retorna 1 se existir o objeto na cache e retorna 0 caso não exista
 */
int checkCache(char **cache, char *name, int n_obj);

/** saveinCache: função que guarda um determinado objeto na cache do nó (importante para pesquisa de objetos nomeados), estando implementado o método Least Recently Used
 * \param **cache: duplo ponteiro para a cache
 * \param *name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \param *n_obj: ponteiro para o inteiro correspondente ao número de objetos presentes na cache
 * \param N: capacidade da cache
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int saveinCache(char **cache, char *name, int *n_obj, int N);

/** printTabExp: função que imprime os objetos que se encontram na cache do nó da aplicação
 * \param **cache: duplo ponteiro para a cache
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 * \param N: capacidade da cache
 */
void printCache(char **cache, int n_obj, int N);

/** clearCache: função que liberta a cache (desaloca a memória alocada nas várias posições da cache correpondente as strings dos objetos)
 * \param **cache: variável do tipo cache_objects corrrespondente à estrutura que guarda informação dos objetos em slots na cache
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 */
void clearCache(char **cache, int n_obj);

/** deleteCacheid: função que elimina da cache todos os objetos que possuem identificador id
 * \param **cache: duplo ponteiro para a cache
 * \param *n_obj: ponteiro para o inteiro correspondente ao número de objetos presentes na cache
 * \param *id: ponteiro para a string correspondente ao identificador dum nó
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int deleteCacheid(char **cache, int *n_obj, char *id);

/** 
 * createinsertCacheAux: função que cria um elemento e adiciona a lista auxiliar (basicamente cria e adiciona um nó a lista simplesmente ligada)
 * \param *head: ponteiro para o primeiro elemento da lista auxiliar (topo da lista)
 * \param *objct: ponteiro para a string correspondente ao nome do objeto
 * \retorna NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int createinsertCacheAux(cache_aux **head_c, char *objct);

/** FreeCacheAuxList: função que liberta a memória alocada pela lista auxiliar
 * \param **head_c: duplo ponteiro para o primeiro elemento da lista auxiliar (topo da lista)
 */
void FreeCacheAuxList(cache_aux **head_c);

/** createCache: cria uma cache de N char* e coloca-os todos a NULL
 * \param N: capacidade da cache
 * \retorna cache criada
 */
char **createCache(int N);

/** freeCache: Liberta a memória de todos os elementos da cache, e dos ponteiros para os elementos da cache
 * \param **cache: duplo ponteiro para a cache
 * \param N: capacidade da cache
 */
void freeCache(char **cache, int N);
#endif
