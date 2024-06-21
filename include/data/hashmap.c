#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

HASHMAP* hashmap_init()
{
    HASHMAP *map = (HASHMAP*)malloc(sizeof(HASHMAP));
    if(map == NULL) return NULL;
    map->len   = MAP_LEN;
    map->table = (HASH_NODE **)malloc(sizeof(HASH_NODE*) * map->len);
    if(map->table == NULL)
    {
        free(map);
        return NULL;
    }
    for (size_t i = 0; i < map->len; i++)
    {
        map->table[i] = NULL;
    }
    return map;
}

HASH_NODE *hash_node_init()
{
    HASH_NODE *node = (HASH_NODE *)malloc(sizeof(HASH_NODE));
    if(node == NULL) return NULL;
    node->key   = NULL;
    node->value = NULL;
    node->next  = NULL;
    return node;
}

unsigned int hash_gen_key(HASHMAP *map, char *key)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int value = 0;
    int i = 0;
    size_t key_len = strlen(key);

    for ( i; i < key_len; i++)
    {
        value += value * 47 + key[i];
    }
    value = value % map->len;
    return value;
}

int hashmap_insert(HASHMAP *map, char* key, void *val)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return -1;

    HASH_NODE *node = hash_node_init();
    if(node == NULL) return -1;
    strcpy(node->key, key);
    

    if(map->table[index] == NULL)
    {
        map->table[index] = node;
        return 0;
    }

    HASH_NODE *curr = map->table[index];
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    curr->next = node;
    return 0;
}


int hashmap_delete(HASHMAP *map, char* key)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return -1;

    HASH_NODE *prev = NULL;
    HASH_NODE *node = map->table[index];

    while (node != NULL && strcmp(node->key, key) != 0)
    {
        prev = node;
        node = node->next;
    }

    if (node == NULL) return -1;

    if(prev == NULL)
    {
        map->table[index] = node->next;
    }else{
        prev->next = node->next;
    }

    free(node->key);
    free(node);
    return 0;
}

HASH_NODE *hashmap_find(HASHMAP *map, char* key)
{
    if(map == NULL || key == NULL) return NULL;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return NULL;

    HASH_NODE *node = map->table[index];
    if(strcmp == 0) return node;

    while (node->next != NULL)
    {
        node = node->next;
        if(strcmp(node->key, key) == 0) return node;
    }
    return NULL;
}

