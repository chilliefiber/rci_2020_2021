#ifndef ERROR_CHECKING_H
#define ERROR_CHECKING_H

#include <stddef.h>
void* safeMalloc(size_t size);
void sendUDP(int fd, char* ip, char* port, char* text, char* addrinfo_error_msg, char* send_error_msg);
void safeGetAddrInfo(char* ip, char* port, struct addrinfo *hints, struct addrinfo **res, char* error_msg);
void* safeCalloc(size_t nmemb, size_t size);
void safeRecvFrom(int fd, char *dgram, size_t len);
#endif
