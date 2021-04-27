#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "nodes.h"
#include "errcheck.h"

int addToList(internals **int_neighbours, viz *new)
{
    internals *aux = *int_neighbours;
    *int_neighbours = malloc(sizeof(internals));
    if(*int_neighbours == NULL)
    return END_EXECUTION;
    
    (*int_neighbours)->this = new;
    (*int_neighbours)->next = aux;
    
    return NO_ERROR;
}

void freeViz(viz **v)
{
    if (*v)
    {
        if (close((*v)->fd))
            fprintf(stderr, "error closing file descriptor of neighbour: %s\n", strerror(errno));
        free(*v);
        *v = NULL;
    }
}

void freeIntNeighbours(internals **int_neighbours)
{
    internals *neigh_aux;
    while (*int_neighbours)
    {
        neigh_aux = *int_neighbours;
        *int_neighbours = (*int_neighbours)->next;
        if (close(neigh_aux->this->fd))
            fprintf(stderr, "error closing file descriptor of internal neighbour: %s\n", strerror(errno));
        free(neigh_aux->this);
        free(neigh_aux);
        neigh_aux = NULL;
    }
    *int_neighbours = NULL;
}


void freeSelf(no *self)
{
    free(self->id);
    free(self->net);
    self->id = NULL; 
    self->net = NULL;
}
