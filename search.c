#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "search.h"
#include "errcheck.h"

int addInterest(list_interest **first_interest, char *obj, int fd)
{
    list_interest *tmp = *first_interest;
    list_interest *new_interest = malloc(sizeof(list_interest));
    if(new_interest == NULL)
        return END_EXECUTION;

    new_interest->obj = malloc(strlen(obj)+1);
    if(new_interest->obj == NULL)
    {
        free(new_interest);
        return END_EXECUTION;
    }

    strcpy(new_interest->obj, obj);
    new_interest->fd = fd;

    if(*first_interest == NULL)
    {
        *first_interest = new_interest;
        new_interest->next = NULL;
    }
    else
    {
        while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = new_interest;
        new_interest->next = NULL;
    }

    return NO_ERROR;
}

void deleteInterest(list_interest **first_interest, char *obj, int fd)
{
    list_interest *tmp;

    if(!strcmp((*first_interest)->obj, obj) && (*first_interest)->fd == fd)
    {
        tmp = *first_interest; 
        *first_interest = (*first_interest)->next;
        free(tmp->obj);
        free(tmp);
    }
    else
    {
        list_interest *curr = *first_interest;

        while(curr->next != NULL)
        {
            if(!strcmp(curr->next->obj, obj) && curr->next->fd == fd)
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp->obj);
                free(tmp);
                break;
            }
            else
                curr = curr->next;
        }
    }
}

void deleteInterestfd(list_interest **first_interest, int fd)
{
    list_interest *tmp;

    if((*first_interest)->fd == fd)
    {
        tmp = *first_interest; 
        *first_interest = (*first_interest)->next;
        free(tmp->obj);
        free(tmp);
    }
    else
    {
        list_interest *curr = *first_interest;

        while(curr->next != NULL)
        {
            if(curr->next->fd == fd)
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp->obj);
                free(tmp);
                break;
            }
            else
                curr = curr->next;
        }
    }
}

int deleteInterestWITHDRAW(list_interest **first_interest, char *id)
{
    char *ident;
    list_interest *interest_aux = *first_interest;
    list_interest *interest_tmp = NULL;
    int we_used_interest_tmp = 0;

    while(interest_aux != NULL)
    {
        ident = NULL;
        ident = getidfromName(interest_aux->obj, ident);
        if(ident == NULL)
            return END_EXECUTION;

        if(!strcmp(ident, id))
        {
            interest_tmp = interest_aux->next;
            we_used_interest_tmp = 1;
            deleteInterest(first_interest, interest_aux->obj, interest_aux->fd);
        }

        if(we_used_interest_tmp)
        {
            interest_aux = interest_tmp;
            interest_tmp = NULL;
            we_used_interest_tmp = 0;
        }
        else
            interest_aux = interest_aux->next;
        free(ident);
    }
    return NO_ERROR;
}

int checkInterest(list_interest *first_interest, char *obj, int fd)
{
    list_interest *aux = first_interest;

    while(aux != NULL)
    {
        // se ja houver um pedido igual do mesmo vizinho
        if(!strcmp(aux->obj, obj) && aux->fd == fd)
        {
            return 1;
        }
        aux = aux->next;
    }
    // se não houver pedido igual
    return 0;
}

void FreeInterestList(list_interest **first_interest)
{
    list_interest *curr = *first_interest;
    list_interest *next;

    while(curr != NULL)
    {
        next = curr->next;
        free(curr->obj);
        free(curr);
        curr = next;
    }

    *first_interest = NULL;
}

char *getidfromName(char *name, char *id)
{
    size_t i, len = 0;
    for(i=0; i<strlen(name); i++)
    {
        if(i>0)
        {
            if(name[i] == '.')
            {
                len = i;
                break;
            }
        }
    }

    id = malloc(len+1);
    if(id == NULL)
        return NULL;

    for(i=0; i<len; i++)
    {
        id[i] = name[i];
    }
    id[len] = '\0';

    return id;
}

char *getConcatString( const char *str1, const char *str2 ) 
{
    char *finalString = NULL;
    size_t n = 0;

    if ( str1 ) n += strlen( str1 );
    if ( str2 ) n += strlen( str2 );

    if ( ( str1 || str2 ) && ( finalString = malloc( n + 1 ) ) != NULL )
    {
        *finalString = '\0';

        if ( str1 ) strcpy( finalString, str1 );
        if ( str2 ) strcat( finalString, str2 );
    }

    return finalString;
}

int checkObjectList(list_objects *head_obj, char *name)
{
    list_objects *aux = head_obj;

    while(aux != NULL)
    {
        if(!strcmp(aux->objct, name))
        {
            return 1;
        }
        aux = aux->next;
    }
    return 0;
}

int createinsertObject(list_objects **head, char *subname, char *id)
{
    int errcode;
    char *str;
    char str_id[150];

    memset(str_id, 0, 150);

    errcode = snprintf(str_id, 150, "%s.", id);
    if (errcode < 0)
    {
        fprintf(stderr, "error in in filling string str_id\n");
        return END_EXECUTION;
    }
    if (errcode >= 150)
    {
        printf("Error: object size is too big\n");
        return NO_ERROR;
    }


    str = getConcatString(str_id, subname);
    if(str == NULL)
        return END_EXECUTION;

    if(checkObjectList(*head,str) == 1)
    {
        printf("Object %s already exists in our own list of objects!\n", str);
    }
    else
    {
        list_objects *tmp = *head;
        list_objects *new_obj = malloc(sizeof(list_objects));
        if(new_obj == NULL)
        {   
            free(str);
            return END_EXECUTION;
        }

        new_obj->objct = malloc(strlen(str)+1);
        if(new_obj->objct == NULL)
        {  
            free(new_obj); 
            free(str);
            return END_EXECUTION;
        }

        strcpy(new_obj->objct, str);

        if(*head == NULL)
        {
            *head = new_obj;
            new_obj->next = NULL;
        }
        else
        {
            while(tmp->next != NULL)
            {
                tmp = tmp->next;
            }
            tmp->next = new_obj;
            new_obj->next = NULL;
        } 
    }
    free(str);

    return NO_ERROR;
}

void printObjectList(list_objects *head_obj)
{
    list_objects *aux = head_obj;
    printf("List of objects:\n");
    while(aux != NULL)
    {
        printf("%s\n",aux->objct);
        aux = aux->next;
    }
}

void FreeObjectList(list_objects **head_obj)
{
    list_objects *curr = *head_obj;
    list_objects *next;

    while(curr != NULL)
    {
        next = curr->next;
        free(curr->objct);
        free(curr);
        curr = next;
    }

    *head_obj = NULL;
}

int checkCache(char **cache, char *name, int n_obj)
{
    int i;

    for(i=0; i<n_obj; i++)
    {
        if(strcmp(cache[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int saveinCache(char **cache, char *name, int *n_obj, int N)
{
    int i;

    if(*n_obj > N)
    {
        for(i=0; i<N-1; i++)
        {
            free(cache[i]);	
            cache[i] = NULL;
            cache[i] = malloc(strlen(cache[i+1])+1);
            if(cache[i] == NULL)
                return END_EXECUTION;
            strcpy(cache[i], cache[i+1]);
        }
        free(cache[i]);
        (*n_obj)--;	
    }

    cache[(*n_obj)-1] = NULL;
    cache[(*n_obj)-1] = malloc(strlen(name)+1);
    if(cache[(*n_obj)-1] == NULL)
        return END_EXECUTION;

    strcpy(cache[(*n_obj)-1], name);

    return NO_ERROR;
}

void printCache(char **cache, int n_obj, int N)
{
    int i;
    printf("Cache (Size:%d)\nNº objects stored: %d\n", N, n_obj);
    for(i=0; i<n_obj; i++)
    {
        printf("Object %d -> %s\n",i+1,cache[i]);
    }
}

void clearCache(char **cache, int n_obj)
{
    int i;

    for(i=0; i<n_obj; i++)
    {
        free(cache[i]);
        cache[i] = NULL;
    }
}

int deleteCacheid(char **cache, int *n_obj, char *id)
{
    int i;
    char *ident;
    int num = *n_obj;
    cache_aux *head_c = NULL, *aux;

    for(i=0; i<(*n_obj); i++)
    {
        ident = NULL;
        ident = getidfromName(cache[i], ident);
        if(ident == NULL)
        {
            FreeCacheAuxList(&head_c);
            return END_EXECUTION;
        }
        if(!strcmp(ident, id))
        {   
            free(cache[i]);
            cache[i] = NULL;
            num--;
        }
        else
        {
            if(createinsertCacheAux(&head_c, cache[i]) == END_EXECUTION)
            {
                FreeCacheAuxList(&head_c);
                return END_EXECUTION;
            }
            free(cache[i]);
            cache[i] = NULL; 
        }
        free(ident);
    }

    i = 0;
    aux = head_c;
    while(aux != NULL)
    {
        cache[i] = malloc(strlen(aux->obj)+1);
        if(cache[i] == NULL)
        {
            FreeCacheAuxList(&head_c);
            return END_EXECUTION;
        }
        strcpy(cache[i], aux->obj);
        aux = aux->next;
        i++;
    }
    FreeCacheAuxList(&head_c);

    *n_obj = num;
    return NO_ERROR;
}

int createinsertCacheAux(cache_aux **head_c, char *objct)
{
    cache_aux *tmp = *head_c;
    cache_aux *new = malloc(sizeof(cache_aux));
    if(new == NULL)
        return END_EXECUTION;

    new->obj = malloc(strlen(objct)+1);
    if(new->obj == NULL)
    {
        free(new);
        return END_EXECUTION;
    }
    strcpy(new->obj, objct);

    if(*head_c == NULL)
    {
        *head_c = new;
        new->next = NULL;
    }
    else
    {
        while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = new;
        new->next = NULL;
    }

    return NO_ERROR;
}

void FreeCacheAuxList(cache_aux **head_c)
{
    cache_aux *curr = *head_c;
    cache_aux *next;

    while(curr != NULL)
    {
        next = curr->next;
        free(curr->obj);
        free(curr);
        curr = next;
    }

    *head_c = NULL;
}

char **createCache(int N)
{
    int i;
    char **cache = malloc(N * sizeof(char*));
    if(cache == NULL)
        return NULL;

    for (i=0; i < N; i++)
        cache[i] = NULL;
    return cache;
}

void freeCache(char **cache, int N)
{
    clearCache(cache, N);
    free(cache);
}
