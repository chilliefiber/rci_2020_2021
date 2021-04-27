#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "errcheck.h" 

extern int errno;

int sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    if (safeGetAddrInfo(ip, port, &hints, &res, addrinfo_error_msg) == ERROR)
        return ERROR;
    if (sendto(fd, text, strlen(text), 0, res->ai_addr, res->ai_addrlen) == -1){  
        fputs(send_error_msg, stderr);
        fprintf(stderr, "error: %s\n", strerror(errno));
        freeaddrinfo(res);
        return ERROR;
    }
    freeaddrinfo(res);
    return NO_ERROR;
}

int safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg)
{
    int errcode;
    errcode = getaddrinfo(ip, port, hints, res);
    if (errcode != 0){
        fputs(error_msg, stderr);
        fprintf(stderr, "error: %s\n", gai_strerror(errcode));
        return ERROR;
    }
    return NO_ERROR;
}

// aqui perguntar ao prof se devemos usar os tipos de argumentos do recvfrom!!!
int safeRecvFrom(int fd, char *dgram, size_t len)
{
    int n;
    struct sockaddr addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);
    n = recvfrom(fd, dgram, len, 0, &addr, &addrlen);
    if (n == -1)
    {
        fputs("Error in recvfrom!\n", stderr);
        fprintf(stderr, "error: %s\n", strerror(errno));
        return END_EXECUTION;
    }
    dgram[n] = '\0';
    return NO_ERROR;
}

// aqui talvez meter uma mensagem de erro individualizada como argumento para sabermos onde no programa crashou
// ver safeGetAddrInfo
int safeTCPSocket(int* fd)
{
    *fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*fd == -1){
        fputs("Error making a TCP socket!\n", stderr);
        fprintf(stderr, "error: %s\n", strerror(errno));
        return ERROR;
    }
    return NO_ERROR;
}


int connectTCP(char *ip, char* port, int fd, char *addrinfo_error_msg, char *connect_error_msg)
{
    struct addrinfo hints, *res;
    int n;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (safeGetAddrInfo(ip, port, &hints, &res, addrinfo_error_msg)==ERROR)
        return ERROR;
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1){
        fputs(connect_error_msg, stderr);
        fprintf(stderr, "error: %s\n", strerror(errno));
        freeaddrinfo(res);
        return ERROR;
    }
    freeaddrinfo(res);
    return NO_ERROR;
}
