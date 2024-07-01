#ifndef CDB_LIST_H
#define CDB_LIST_H

#include <stddef.h>

typedef struct LIST LIST;

struct LIST
{
    LIST *next;
    LIST *prev;
};

LIST *list_init();
int list_insert_after(LIST *list, LIST *node);
int list_insert_before(LIST *list, LIST *node);
int list_del(LIST *list);
int list_rm_node(LIST *list);


#define LIST_FOR_EACH(curs, list) \
    for (curs = list->next; curs; curs = curs->next)\

// object containing this list
#define LIST_PARENT(node, parent_type, prop_name)   \
    ((parent_type *)((char *)node - offsetof(parent_type, prop_name))) \


#endif