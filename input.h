#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>


enum instr {ERR, JOIN_ID, JOIN_LINK, CREATE, GET, ST, SR, SC, LEAVE, EXIT};


void helpMenu(void);

char* getParam(char *input);

char *readCommand(enum instr *instr_code);

int validate_number(char *str);

int isIP(char *ip); 

int isPort(char port[]);

void* safeMalloc(size_t size);

void* safeCalloc(size_t nmemb, size_t size);




#endif
