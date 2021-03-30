#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>


enum instr {ERR, JOIN_ID, JOIN_SERVER_DOWN, JOIN_LINK, CREATE, GET, ST, SR, SC, LEAVE, EXIT};


void helpMenu(void);

void warnOfTrashReceived(char* warning, char *trash);
char* getParam(char *input);

char *readCommand(enum instr *instr_code);

int checkDigit(char word[]);

int isIP(char ip[]); 

int isPort(char port[]);

int isName(char name[]);





#endif
