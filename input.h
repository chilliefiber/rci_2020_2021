#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>

// enum com as várias instruções/operações possíveis referentes aos comandos introduzidor pelo utilizador (stdin)
enum instr {ERR, JOIN_ID, JOIN_SERVER_DOWN, JOIN_LINK, CREATE, GET, ST, SR, SC, LEAVE, EXIT};

/**
 * helpMenu: função que mostra ao utilizador depois de entrar no programa a lista de comandos
 */
void helpMenu(void);

/**
 * warnOfTrashReceived: função que alerta o utilizador se recebemos algum lixo pela string trash (usada na parte de comunicação por UDP)
 * \param *warning: ponteiro para a string warning
 * \param *trash: ponteeiro para a string trash
 */
void warnOfTrashReceived(char* warning, char *trash);

/**
 * getParam: função que processa os parâmetros introduzidos após o comando/intrução (join, create, get)
 * \param *input: ponteiro para a string input
 * \param *errcode: ponteiro para o int errcode (verificação de erros)
 * \return um ponteiro para a string que conterá os parâmetros introduzidos à frente da instrução
 */
char* getParam(char *input, int *errcode);

/**
 * readCommand: função que lê do terminal (pelo stdin) o comando introduzido pelo utilizador
 * \param *instr_code: ponteiro para uma variável do tipo instr
 * \param *errcode: ponteiro para o int errcode (verificação de erros)
 * \return um ponteiro para a string que conterá os parâmetros introduzidos
 */
char *readCommand(enum instr *instr_code, int *errcode);

/**
 * checkDigit: função que recebe uma string e verifica se esta é só composta unicamente por digitos
 * \param word: string
 * \return 1 se a string tiver só digitos, return 0 caso contrário
 */
int checkDigit(char word[]);

/**
 * isIP: função que recebe uma string e verifica se esta corresponde a um endereço IPv4 válido
 * \param ip: string
 * \return 1 se a string corresponde a um endereço IPv4 válido, return 0 caso contrário
 */
int isIP(char ip[]); 

/**
 * isPort: função que recebe uma string e verifica se esta corresponde a um porto válido
 * \param port: string
 * \return 1 se a string corresponde a um porto válido, return 0 caso contrário
 */
int isPort(char port[]);

/**
 * isName: função que recebe uma string e verifica se esta corresponde a um nome dum objeto válido (i.e id.subname)
 * \param name: string
 * \return 1 se a string corresponde a um nome de objeto válido, return 0 caso contrário
 */
int isName(char name[]);

/**
 * countblankSpace: função que conta o número de espaços entre palavras
 * terminal: string que contem tudo o que o utilizador escreveu como input no terminal
 */
int countblankSpace(char terminal[]);

/**
 * checkEntryArgs: função que verifica os argumentos de entrada ao correr o programa no terminal
 * \param **argv: duplo ponteiro para argv (array de strings)
 * \param argc: número de argumentos de entrada
 * \param **IP: duplo ponteiro para a string IP (endereço IP do nó da aplicação)
 * \param **TCP: duplo ponteiro para a string TCP (porto TCP em que o nó ficará à escuta)
 * \param **regIP: duplo ponteiro para a string regIP (endereço IP do servidor de nós)
 * \param **regUDP: duplo ponteiro para a string regUDP (porto UDP do servidor de nós)
 * \param *N: ponteiro para a capacidade da cache N
 * \return NO_ERROR se não houver erro e END_EXECUTION se houver erro na alocação dinâmica de memória
 */
int checkEntryArgs(char **argv, int argc, char **IP, char **TCP, char **regIP, char **regUDP, int *N);
#endif
