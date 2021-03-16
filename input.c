#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "input.h"

void helpMenu(void)
{
	printf("Commands: \n");
	printf("join id \n"); //Entrada de um nó na rede com identificador id
	printf("join id bootIP bootTCP\n"); //Entrada de um nó na rede com identificador id. É passado o contacto de um nó da rede, através dos parâmetros bootIP e bootTCP, ao qual o nó se deverá ligar sem interrogar o servidor de nós
	printf("create subname\n"); //É criado um objeto cujo nome será da forma id.subname, em que id é o identificador do nó
	printf("get name\n"); //Inicia-se a pesquisa do objeto com o nome name. Este nome será da forma id.subname, em que id é o identificador de um nó e subname é o sub-nome do objeto atribuído pelo nó com identificador id
	printf("show topology (st)\n"); //Mostra os contactos do vizinho externo e do vizinho de recuperação
	printf("show routing (sr)\n"); //Mostra a tabela de expedição do nó
	printf("show cache (sc)\n"); //Mostra os nomes dos objetos guardados na cache
	printf("leave\n"); //Saída do nó da rede
	printf("exit\n"); //Fecho da aplicação
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
	char terminal[128], command[14], bootIP[INET_ADDRSTRLEN], bootTCP[NI_MAXSERV], lixo[2], second[20];
	int size_input = 0;
	
	fgets(terminal,128,stdin);
	size_input = sscanf(terminal,"%s %s %s %s %s", command, second, bootIP, bootTCP, lixo);
	*instr_code = ERR;

	if(size_input <= 0)
	{
		printf("Error in input\n");
		return NULL;
	}
	
	if(size_input == 5)
	{
		printf("Invalid command\n");
		return NULL;
	}
	
	if(strcmp("join",command) == 0)
	{
		if(size_input == 2)
		{
			if(checkDigit(second) == 1)
			{
				*instr_code = JOIN_ID;
				
				return getParam(terminal);
			}
		}
		if(size_input == 4)
		{
			if(checkDigit(second) == 1 && isPort(bootTCP) == 1)
			{
				*instr_code = JOIN_LINK;
				return getParam(terminal);
			}
		}
		else
		{
			printf("Wrong format! Commands: join id, join id bootIP bootTCP\n");
		}	
	}
	else if(strcmp("create",command) == 0)
	{
		if(size_input == 2)
		{
			*instr_code = CREATE;
		
			return getParam(terminal);
		}
		else
		{
			printf("Wrong format! Command: create subname\n");
		}
	}
	else if(strcmp("get",command) == 0)
	{
		if(size_input == 2)
		{
			*instr_code = GET;
				
			return getParam(terminal);
		}
		else
		{
			printf("Wrong format! Command: get name\n");
		}
	}
	else if(strcmp("show topology",command) == 0 || strcmp("st",command) == 0)
	{
		if(size_input == 1)
		{
			*instr_code = ST;
		}
		else
		{
			printf("Wrong format in command! Command: show topology or st\n");
		}
	}
	else if(strcmp("show routing",command) == 0 || strcmp("sr",command) == 0)
	{
		if(size_input == 1)
		{
			*instr_code = SR;
		}
		else
		{
			printf("Wrong format in command! Command: show routing or sr\n");
		}
	}
	else if(strcmp("show cache",command) == 0 || strcmp("sc",command) == 0)
	{
		if(size_input == 1)
		{
			*instr_code = SC;
		}
		else
		{
			printf("Wrong format in command! Command: show cache or sc\n");
		}
	}
	else if(strcmp("leave",command) == 0)
	{
		if(size_input == 1)
		{
			*instr_code = LEAVE;
		}
		else
		{
			printf("Wrong format in command! Command: leave\n");
		}
	}
	else if(strcmp("exit",command) == 0)
	{
		if(size_input == 1)
		{
			*instr_code = EXIT;
		}
		else
		{
			printf("Wrong format in command! Command: exit\n");
		}
	}
	
	if(*instr_code == ERR)
	{
		printf("Error in command input!\n");
		return NULL;
	}
}

int checkDigit(char word[])
{
  int i = 0;
	
  // verificar se todos os caracteres de uma string não números entre 0 e 9
  // se sim retornar 1, se não retornar 0 e imprimir mensagem de erro
  for(i=0; i<strlen(word); i++)
	if(word[i] < '0' || word[i] > '9'){
          printf("Error every char must be a positive number\n");
	  return 0;
        }
  return 1;
}

int isPort(char port[])
{
  int val = 0;
  
  val = atoi(port);
  // verificar se porto está dentro dos limites permitidos, se sim 
  // retornar 1, se não retornar 0 e imprimir mensagem de erro
  if(checkDigit(port) && val >= 1025 && val <= 65536)
    return 1;	
  printf("Error the inserted port must be inside the valid range\n");
  return 0;
}

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
