#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errcheck.h" 

extern int errno;
void* safeMalloc(size_t size)
{
    // alocar memória de uma variável do tipo desejado
    // mostrar mensagem de erro caso tal não seja conseguido
    void* p = malloc(size);
    if (p == NULL){
        fputs("Memory error - malloc\n", stdout);
        fprintf(stderr, "error: %s\n", strerror(errno));
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
        fputs("Memory error - calloc\n", stdout);
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

// talvez fazer comno eu fiz no outro projeto
// em que meti isto como int
void sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg)
{
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof hints);
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_DGRAM;
  safeGetAddrInfo(ip, port, &hints, &res, addrinfo_error_msg);
  if (sendto(fd, text, strlen(text), 0, res->ai_addr, res->ai_addrlen) == -1){  
    fputs(send_error_msg, stdout);
    fprintf(stderr, "error: %s\n", strerror(errno));
    freeaddrinfo(res);
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(res);
}

void safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg)
{
  size_t n;
  n = getaddrinfo(ip, port, hints, res);
  if (n != 0){
    fputs(error_msg, stdout);
    fprintf(stderr, "error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

// aqui perguntar ao prof se devemos usar os tipos de argumentos do recvfrom!!!
void safeRecvFrom(int fd, char *dgram, size_t len)
{
    int n;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);
    n = recvfrom(fd, dgram, len, 0, &addr, &addrlen);
    if (n == -1)
    {
        fputs("Error in recvfrom!\n", stdout);
        fprintf(stderr, "error: %s\n", strerror(errno));
        exit(-1);
    }
    dgram[n] = '\0';
}
