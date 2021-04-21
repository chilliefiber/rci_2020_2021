#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "input.h"
#include "errcheck.h"

void helpMenu(void)
{
	printf("\nCommands:\n");
	printf("join net id \n"); //Entrada de um nó na rede net com identificador id
	printf("join net id bootIP bootTCP\n"); //Entrada de um nó na rede net com identificador id. É passado o contacto de um nó da rede, através dos parâmetros bootIP e bootTCP, ao qual o nó se deverá ligar sem interrogar o servidor de nós
	printf("create subname\n"); //É criado um objeto cujo nome será da forma id.subname, em que id é o identificador do nó
	printf("get name\n"); //Inicia-se a pesquisa do objeto com o nome name. Este nome será da forma id.subname, em que id é o identificador de um nó e subname é o sub-nome do objeto atribuído pelo nó com identificador id
	printf("show topology (st)\n"); //Mostra os contactos do vizinho externo e do vizinho de recuperação
	printf("show routing (sr)\n"); //Mostra a tabela de expedição do nó
	printf("show cache (sc)\n"); //Mostra os nomes dos objetos guardados na cache
	printf("leave\n"); //Saída do nó da rede
	printf("exit\n\n"); //Fecho da aplicação
}

void warnOfTrashReceived(char* warning, char *trash)
{
    fputs(warning, stdout);
    fputs("This is the trash\n", stdout);
    fputs(trash, stdout);
    fputs("\n That was it\n", stdout);
}
char* getParam(char *input)
{
  // ignorar leading whitespace
  while (isspace(*input)) input++;
  // avançar pelo comando
  while(!isspace(*input)) input++;
  // avançar os espaços entre o comando e o primeiro parâmetro
  while (isspace(*input)) input++;
  // copiar parametros para string nova
  char *params = safeMalloc(strlen(input) + 1);
  strcpy(params, input);
  return params;
}

char *readCommand(enum instr *instr_code)
{
	char terminal[128], command[14], bootIP[NI_MAXHOST], bootTCP[NI_MAXSERV], second[64], id[64];
    	memset(terminal, 0, 128);
    	memset(command, 0, 14);
    	memset(bootIP, 0, NI_MAXHOST);
    	memset(bootTCP, 0, NI_MAXSERV);
    	memset(second, 0, 64);
    	memset(id, 0, 64);
	int size_input = 0;
	
	fgets(terminal,128,stdin);
	size_input = sscanf(terminal,"%s %s %s %s %s", command, second, id, bootIP, bootTCP);
	*instr_code = ERR;
	
	if(size_input<=0)
	{
		printf("Error in command input!\n");
		return NULL;
	}
	
	if(strcmp("join",command) == 0)
	{
		if(size_input == 3 && (countblankSpace(terminal) == 2))
		{
		    *instr_code = JOIN_ID;
			return getParam(terminal);
		}
		else if(size_input == 5 && (countblankSpace(terminal) == 4))
		{
			if(isIP(bootIP) == 1 && isPort(bootTCP) == 1)
			{
				*instr_code = JOIN_LINK;
				
				return getParam(terminal);
			}
			if(isIP(bootIP) != 1)
			{
				printf("Invalid IPv4 address!\n");
			}
			if(isPort(bootTCP) != 1)
			{
				printf("Invalid Port!\n");
			}
		}
		else
		{
			printf("Wrong format in command!\nCorrect formats:\njoin<space>net<space>id<ENTER>\njoin<space>net<space>id<space>bootIP<space>bootTCP<ENTER>\n");
		}	
	}
	else if(strcmp("create",command) == 0)
	{
		if(size_input == 2 && (countblankSpace(terminal) == 1))
		{
			*instr_code = CREATE;
		
			return getParam(terminal);
		}
		else
		{
			printf("Wrong format in command!\nCorrect format:\ncreate<space>subname<ENTER>\n");
		}
	}
	else if(strcmp("get",command) == 0)
	{
		if(size_input == 2 && (countblankSpace(terminal) == 1))
		{
			if(isName(second) == 1)
			{
				*instr_code = GET;
				
				return getParam(terminal);
			}
			else
			{
				printf("Invalid name! Name format must be: id.subname\n");
			}
		}
		else
		{
			printf("Wrong format in command!\nCorrect format:\nget<space>name<ENTER>\n");
		}
	}
	else if((strcmp("show",command) == 0 && strcmp("topology",second) == 0) || strcmp("st",command) == 0)
	{
		if((size_input == 1 && !strcmp("st",command) && (countblankSpace(terminal) == 0)) || (size_input == 2 && !strcmp("show",command) && !strcmp("topology",second) && (countblankSpace(terminal) == 1)))
		{
			*instr_code = ST;
		}
		else
		{
			printf("Wrong format in command!\nCorrect formats:\nshow<space>topology<ENTER>\nst<ENTER>\n");
		}
	}
	else if((strcmp("show",command) == 0 && strcmp("routing",second) == 0) || strcmp("sr",command) == 0)
	{
		if((size_input == 1 && !strcmp("sr",command) && (countblankSpace(terminal) == 0)) || (size_input == 2 && !strcmp("show",command) && !strcmp("routing",second) && (countblankSpace(terminal) == 1)))
		{
			*instr_code = SR;
		}
		else
		{
			printf("Wrong format in command!\nCorrect formats:\nshow<space>routing<ENTER>\nsr<ENTER>\n");
		}
	}
	else if((strcmp("show",command) == 0 && strcmp("cache",second) == 0) || strcmp("sc",command) == 0)
	{
		if((size_input == 1 && !strcmp("sc",command) && (countblankSpace(terminal) == 0)) || (size_input == 2 && !strcmp("show",command) && !strcmp("cache",second) && (countblankSpace(terminal) == 1)))
		{
			*instr_code = SC;
		}
		else
		{
			printf("Wrong format in command!\nCorrect formats:\nshow<space>cache<ENTER>\nsc<ENTER>\n");
		}
	}
	else if(strcmp("leave",command) == 0)
	{
		if(size_input == 1 && (countblankSpace(terminal) == 0))
		{
			*instr_code = LEAVE;
		}
		else
		{
			printf("Wrong format in command!\nCorrect format:\nleave<ENTER>\n");
		}
	}
	else if(strcmp("exit",command) == 0)
	{
		if(size_input == 1 && (countblankSpace(terminal) == 0))
		{
			*instr_code = EXIT;
		}
		else
		{
			printf("Wrong format in command!\nCorrect format:\nexit<ENTER>\n");
		}
	}
	else
	{
		if(*instr_code == ERR)
		{
			printf("Error in command input!\n");
		}
	}
		return NULL;
}

int checkDigit(char word[])
{
  int i = 0;
	
  // verificar se todos os caracteres de uma string não números entre 0 e 9
  // se sim retornar 1, se não retornar 0 e imprimir mensagem de erro
  for(i=0; i<strlen(word); i++)
	if(word[i] < '0' || word[i] > '9'){

	  return 0;
        }
  return 1;
}

int isIP(char ip[])
{
	unsigned long addr_ip = 0;
	
	if(inet_pton(AF_INET, ip, &addr_ip) == 0)
	{
		return 0 ;
	}
	return 1;
}

int isPort(char port[])
{
  int val = 0;
  
  val = atoi(port);
  // verificar se porto está dentro dos limites permitidos, se sim 
  // retornar 1, se não retornar 0 e imprimir mensagem de erro
  if(checkDigit(port) && val >= 1025 && val <= 65535)
    return 1;	

  return 0;
}

int isName(char name[])
{ 
	int i, count_point = 0;
	
	if(name[0] == '.' && name[1] == '\n')
	{
		return 0;
	}
	
	for(i=0; i<strlen(name); i++)
	{
		if(i>0)
		{
			if((name[i] == '.' && name[i+1] != '\n') && (name[i] == '.' && name[i+1] != '\0'))
			{
				count_point++;
			}
		}
	}
	
	if(count_point < 1)
	{
		return 0;
	}
	
	return 1;
}

int countblankSpace(char terminal[])
{
	int i, counter = 0;
	for(i=0; terminal[i] != '\0'; i++)
	{
		if(terminal[i] == ' ')
		counter++;
	}
	return counter;
}

