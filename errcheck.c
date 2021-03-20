#include <stdlib.h>
#include <stdio.h>

#include "errcheck.h" 

void* safeMalloc(size_t size)
{
    
    // alocar memória de uma variável do tipo desejado
    // mostrar mensagem de erro caso tal não seja conseguido
    void* p = malloc(size);
    if (p == NULL){
        printf("Memory error\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

void* safeCalloc(size_t nmemb, size_t size)
{
    
    // alocar memória de uma variável do tipo desejado
    // mostrar mensagem de erro caso tal não seja conseguido
    void* p = calloc(nmemb, size);
    if (p == NULL){
        printf("Memory error\n");
        exit(EXIT_FAILURE);
    }
    return p;
}
