void parseNodeListRecursive(char* datagram, int *num_nodes, node_list **list)
{
    int rvalue;
    node_list *this = safeMalloc(sizeof(node_list));
    this.next = NULL;
    rvalue = sscanf(line,"%s %s\n", this.node_IP, this.node_port);
    if (rvalue == EOF){
        fprintf(stderr, "error in parseNodeListRecursive(): %s\n", strerror(errno));
    }
    // neste caso houve um erro mas devido a uma mensagem mal formatada
    // sendo assim vamos devolver uma lista a NULL para sinalizar esse erro, mas
    // não paramos o programa
    else if(rvalue != 2){
        *list = NULL;
        return;
    }
    node_list *aux = *list;
    if (aux == NULL)
        *list = this;
    else
    {
        // ir até ao último elemento da lista 
        for (aux; aux->next != NULL; aux = aux->next);
        // colocar o elemento recém-criado 
        aux->next = this;
    }
    *num_nodes = (*num_nodes) + 1;
    int ix =0;
    char c = datagram[ix];
    // obrigatoriamente tem um \n devido ao sscanf, se aquilo
    // não tiver bugs esquisitos com whitespace ser igual todo
    // um ao outro e o \n não existir mas estar la outro espaço qq, ou nenhum
    while ((c=datagram[ix]) != '\n')
        ix++;
    // chegámos ao fim da linha atual
    // se o próximo caractere for \0, lemos o dgram todo
    // caso contrário, vamos ler mais uma linha
    if (datagram[ix + 1] != '\0')
        parseNodeListRecursive(datagram + ix + 1, num_nodes, list);

}


// datagram é a mensagem recebida, terminada com \0
// net é o número da rede para a qual estamos à espera de receber a lista dos nós
// nodeslist_received é um booleano
char* isNodesList(char* datagram, unsigned int net, char *nodeslist_received){
    char message_until_newline[100]; // passar o 100 para uma constante, depois passar o 100
    char message_supposed_to_recv[100];
    *nodeslist_received = 0;
    int error_flag;
    error_flag = snprintf(message_supposed_to_recv, 100,  "NODESLIST %u\n", net);
    // na especificação do C em certo tipo de erros ele devolve NULL
    // segundo a man page. Talvez nao seja necessaria a verificação para
    // ser maior de 100, mas vi em https://www.cplusplus.com/reference/cstdio/snprintf/ 
    // que punham também
    //
    // Depois colocar nas funções uma flag para que quando haja erros ele faz
    // exit no main, limpando a memória alocada toda (se o prof quiser, se bem
    // que é desnecessário)
    if (message_supposed_to_recv == NULL || error_flag < 0 || error_flag => 100)
        fprintf(stderr, "error in isNodesList(): %s\n", strerror(errno));
    char c = datagram[0];
    unsigned int ix=0;
    while(c != '\0')
    {
        message_until_newline[ix] = c;
        if (c == '\n')
        {
            // se a primeira linha lida foi a do comando recebemos a mensagem correta
            if (!strcmp(message_until_newline, message_supposed_to_recv)){
                *nodeslist_received = 1;
                // se ainda há mais linhas para ler, então a lista não vem vazia
                // nesse caso, devolvemos uma string que aponta para o início
                // da próxima linha. Caso contrário devolvemos NULL. Graças à flag
                // nodes_list_received conseguimos perceber se o NULL se deve a uma
                // lista vazia ou a uma mensagem mal formatada
                if (datagram[ix + 1] != '\0')
                    return datagram + ix + 1;
            }
            return NULL;
        }
        ix = ix + 1;
        // caso haja 100 caracteres antes do \n, então assumimos que a mensagem estava mal formatada, ou nao era um
        // NODESLIST
        if (ix == 100)
            return NULL;
        c = datagram[ix];
    }
    // chegámos ao \0 mas não chegámos a 100 caracteres nem ao \n
    return NULL;
}
/*
node_list *parseNodelist(char* datagram, int *num_nodes)
{
    int datagram_ix = 0, line_ix=0;
    node_list* list = NULL;
    *num_nodes = -1;
    char c = datagram[datagram_ix];
    char line[150]; // colocar aqui uma variavel com #DEFINE 150
    while (c != '\0')
    {
        // quando num_nodes == -1 estamos a ler o comando e nao um par IP/porto
        if (*num_nodes != -1)
        {
            line[line_ix] = c;
            line_ix++;
        }
        if (c == '\n')
        {
            (*num_nodes)++;
            // neste caso ou é zero, e portanto lemos apenas o comando
            // ou devemos fazer um elemento da lista de nós
            if (*num_nodes)
            {   
                line[line_ix] = '\0';
                addLineToList(&list, line);
                line_ix = 0; // reiniciar o indice, porque vamos começar a ler uma linha nova
            }
        }
        datagram_ix++;
        c = datagram[datagram_ix];
    }
    return list;
}

void addLineToList(node_list **list, char *line)
{
    node_list *this = safeMalloc(sizeof(node_list));
    this.next = NULL;
    node_list *aux = *list;
    sscanf(line,"%s %s ", this.node_IP, this.node_port);
    if (*list == NULL)
        *list = this;
    else
    {
        // go to last element of list
        for (aux; aux->next != NULL; aux = aux->next);
        // place newly created element in list
        aux->next = this;
    }
}
*/

