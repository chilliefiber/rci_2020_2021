#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "routing.h"
#include "errcheck.h"

int createinsertTabEntry(tab_entry **first_entry, char *id_dst, int fd)
{
    tab_entry *tmp = *first_entry;
    tab_entry *new_entry = malloc(sizeof(tab_entry));
    if(new_entry == NULL)
        return END_EXECUTION;

    new_entry->id_dest = malloc(strlen(id_dst)+1);
    if(new_entry->id_dest == NULL)
    {
        free(new_entry);
        return END_EXECUTION;
    }

    strcpy(new_entry->id_dest, id_dst);
    new_entry->fd_sock = fd;

    if(*first_entry == NULL)
    {
        *first_entry = new_entry;
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

    return NO_ERROR;
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
