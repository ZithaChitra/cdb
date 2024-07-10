#include <stdlib.h>
#include "list.h"


LIST *list_init()
{
    LIST *list = (LIST *)malloc(sizeof(LIST));
    if(list == NULL) return NULL;
    list->next = NULL;
    list->prev = NULL;
    return list;
}

int list_del(LIST *list)
{
    if(list == NULL) return -1;
    LIST *curs = NULL;
    LIST *tmp  = NULL;

    for (curs = list; curs != NULL;)
    {
        tmp  = curs;
        curs = curs->next;
        free(tmp);
    }
    return 0;
}

int list_insert_after(LIST *list, LIST *node)
{
    if(list == NULL || node == NULL) return -1;
    node->next = list->next;
    if(node->next != NULL) node->next->prev = node;
    node->prev = list;
    list->next = node;
    return 0;
}


int list_insert_before(LIST *list, LIST *node)
{
    if(list == NULL || node == NULL) return -1;
    list->prev ? list->prev->next = node : NULL;
    node->prev = list->prev;
    node->next = list;
    list->prev = node;
    return 0;
}


int list_rm_node(LIST *list)
{
    if(list == NULL) return -1;
    if(list->prev)  list->prev->next = list->next;
    if(list->next ) list->next->prev = list->prev;
    return 0;
}

void list_traverse(LIST *list)
{
}