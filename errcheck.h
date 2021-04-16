#ifndef ERROR_CHECKING_H
#define ERROR_CHECKING_H

#define END_EXECUTION 0
#define LEAVE_NETWORK 1
#define NO_ERROR 2

#include <stddef.h>
#include <netdb.h>

void* safeMalloc(size_t size);
void sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg);
void safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg);
void* safeCalloc(size_t nmemb, size_t size);
void safeRecvFrom(int fd, char *dgram, size_t len);
void safeTCPSocket(int* fd);
void connectTCP(char *ip, char* port, int fd, char *addrinfo_error_msg, char *connect_error_msg);
#endif
