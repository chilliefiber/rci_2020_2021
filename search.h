#ifndef SEARCH_H
#define SEARCH_H

// estrutura auxiliar para cache (caso WITHDRAW retirar elementos com o id)
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

list_interest *addInterest(list_interest *first_interest, char *obj, int fd);

void deleteInterest(list_interest **first_interest, char *obj, int fd);

void deleteInterestfd(list_interest **first_interest, int fd);

void deleteInterestWITHDRAW(list_interest *first_interest, char *id);

int checkInterest(list_interest *first_interest, char *obj, int fd);

void FreeInterestList(list_interest **first_interest);

/**
 * getidfromName: função que extrai do nome introduzido pelo comando "get" o ientificador do nó destino para a pesquisa do objeto
 * \param user_input: ponteiro para a string correspondente ao nome introduzido pelo comando "get"
 * \param id: ponteiro para a string correspondente ao identificador do nó que será retornado no final desta função
 */
char *getidfromName(char *user_input, char *id);


/** getConcatString: função que concatena duas strings numa só e retorna no final o ponteiro para string resultante (utilizada na criação de objetos para juntar id ao subnome)
 * \param str1: ponteiro para a primeira string (corresponderá ao identificador do nó)
 * \param str2: ponteiro para a segunda string (corresponderá ao subnome do objeto)
 */
char *getConcatString( const char *str1, const char *str2);

/** checkObjectList: função que verifica se existe um determinado objeto na lista de objetos nomeados do nó (importante para pesquisa de objetos nomeados)
 * \param head_obj: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 * \param name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \retorna 1 se existir o objeto na lista e retorna 0 caso não exista
 */
int checkObjectList(list_objects *head_obj, char *name);

/**
 * createinsertObject: função que cria um objeto e adiciona a lista de objetos nomeados do nó (basicamente cria e adiciona um nó a lista simplesmente ligada) e retorna o topo da lista atualizado no final
 * \param head: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 * \param subname: ponteiro para a string correspondente ao subnome do objeto introduzido no comando "create"
 * \param id: ponteiro para a string correspondente ao identificador do nó da aplicação 
 */
list_objects *createinsertObject(list_objects *head, char *subname, char *id);

/** printTabExp: função que imprime a lista de objetos nomeados do nó da aplicação
 * \param head_obj: ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 */
void printObjectList(list_objects *head_obj);

/** FreeObjectList: função que liberta a lista de objetos nomeados (desaloca a memória alocada pelos objetos(nós) da lista)
 * \param head_obj: duplo ponteiro para o primeiro objeto da lista de objetos (topo da lista)
 */
void FreeObjectList(list_objects **head_obj);

/** checkCache: função que verifica se existe um determinado objeto na cache do nó (importante para pesquisa de objetos nomeados)
 * \param cache: variável do tipo cache_objects corrrespondente à estrutura que guarda informação dso objetos em slots da cache
 * \param name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 * \retorna 1 se existir o objeto na cache e retorna 0 caso não exista
 */
int checkCache(char **cache, char *name, int n_obj);

/** saveinCache: função que guarda um determinado objeto na cache do nó (importante para pesquisa de objetos nomeados), estando implementado o método Least Recently Used
 * \param cache: variável do tipo cache_objects corrrespondente à estrutura que guarda informação dos objetos em slots na cache
 * \param name: ponteiro para a string correspondente ao nome do objeto a verificar
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 * \param N: capacidade da cache
 * \retorna 1 se existir o objeto na cache e retorna 0 caso não exista
 */
int saveinCache(char **cache, char *name, int n_obj, int N);

/** printTabExp: função que imprime os objetos que se encontram na cache do nó da aplicação
 * \param cache: variável do tipo cache_objects corrrespondente à estrutura que guarda informação dos objetos em slots na cache
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 * \param N: capacidade da cache
 */
void printCache(char **cache, int n_obj, int N);

/** FreeCache: função que liberta a cache (desaloca a memória alocada nas várias posições da cache correpondente as strings dos objetos)
 * \param cache: variável do tipo cache_objects corrrespondente à estrutura que guarda informação dos objetos em slots na cache
 * \param n_obj: inteiro correspondente ao número de objetos presentes na cache
 */
void FreeCache(char **cache, int n_obj);

int deleteCacheid(char **cache, int n_obj, char *id);

cache_aux *createinsertCacheAux(cache_aux *head_c, char *objct);

void FreeCacheAuxList(cache_aux **head_c);

#endif
