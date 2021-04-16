#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "routing.h"
#include "errcheck.h"
#include "tcp.h"

tab_entry *createinsertTabEntry(tab_entry *first_entry, char *id_dst, int fd)
{
    tab_entry *tmp = first_entry;
    tab_entry *new_entry = safeMalloc(sizeof(tab_entry));

    new_entry->id_dest = safeMalloc(strlen(id_dst)+1);
    strcpy(new_entry->id_dest, id_dst);
    new_entry->fd_sock = fd;

    if(first_entry == NULL)
    {
        first_entry = new_entry;
        new_entry->next = NULL;
    }
    else
    {
        while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = new_entry;
        new_entry->next = NULL;
    }

    return first_entry;
}

void deleteTabEntryid(tab_entry **first_entry, char *id_out)
{
    tab_entry *tmp;

    if(!strcmp((*first_entry)->id_dest, id_out))
    {
        tmp = *first_entry; 
        *first_entry = (*first_entry)->next;
        free(tmp->id_dest);
        free(tmp);
    }
    else
    {
        tab_entry *curr = *first_entry;

        while(curr->next != NULL)
        {
            if(!strcmp(curr->next->id_dest, id_out))
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp->id_dest);
                free(tmp);
                break;
            }
            else
                curr = curr->next;
        }
    }
}

void deleteTabEntryfd(tab_entry **first_entry, int fd_out)
{
    tab_entry *tmp;

    if((*first_entry)->fd_sock == fd_out)
    {
        tmp = *first_entry; 
        *first_entry = (*first_entry)->next;
        free(tmp->id_dest);
        free(tmp);
    }
    else
    {
        tab_entry *curr = *first_entry;

        while(curr->next != NULL)
        {
            if(curr->next->fd_sock == fd_out)
            {
                tmp = curr->next;
                curr->next = curr->next->next;
                free(tmp->id_dest);
                free(tmp);
                break;
            }
            else
                curr = curr->next;
        }
    }
}

void writeAdvtoEntryNode(tab_entry *first_entry, int errcode, char *buffer, int fd)
{
    tab_entry *aux = first_entry;

    while(aux != NULL)
    {
        errcode = snprintf(buffer, 150, "ADVERTISE %s\n", aux->id_dest);  
        if (buffer == NULL || errcode < 0 || errcode >= 150)
        {
            fprintf(stderr, "error in ADVERTISE TCP message creation\n");
            exit(-1);  
        }
        writeTCP(fd, strlen(buffer), buffer);
        aux = aux->next;
    }
}

int checkTabEntry(tab_entry *first_entry, char *id)
{
    tab_entry *aux = first_entry;

    while(aux != NULL)
    {
        if(!strcmp(aux->id_dest, id))
        {
            return 1;
        }
        aux = aux->next;
    }
    return 0;
}

void printTabExp(tab_entry *first_entry)
{
    tab_entry *aux = first_entry;
    printf("Routing Table:\n");
    while(aux != NULL)
    {
        printf("%s, %d\n",aux->id_dest,aux->fd_sock);
        aux = aux->next;
    }
}

void FreeTabExp(tab_entry **first_entry)
{
    tab_entry *curr = *first_entry;
    tab_entry *next;

    while(curr != NULL)
    {
        next = curr->next;
        free(curr->id_dest);
        free(curr);
        curr = next;
    }

    *first_entry = NULL;
}
