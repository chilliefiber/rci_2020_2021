#ifndef ERROR_CHECKING_H
#define ERROR_CHECKING_H

#include <stddef.h>
void* safeMalloc(size_t size);

void* safeCalloc(size_t nmemb, size_t size);
#endif
