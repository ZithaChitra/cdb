#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hashmap.h"

int free_and_null(void **data)
{
    if(*data == NULL) return -1;
    free(*data);
    *data = NULL;
    return 0;
}

HASHMAP* hashmap_init()
{
    HASHMAP *map = (HASHMAP*)malloc(sizeof(HASHMAP));
    if(map == NULL) return NULL;
    map->len   = MAP_LEN;
    map->table = (HASHNODE **)malloc(sizeof(HASHNODE*) * map->len);
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

HASHNODE *hashnode_init()
{
    HASHNODE *node = (HASHNODE *)malloc(sizeof(HASHNODE));
    if(node == NULL) return NULL;
    node->key   = NULL;
    node->value = NULL;
    node->next  = NULL;
    return node;
}

int hashnode_del(HASHNODE *node)
{
    if (node == NULL) return -1;
    free(node->key);
    free(node->value);
    free(node);
    return 0;
}

unsigned int hash_gen_key(HASHMAP *map, char *key)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int value = 0;
    int i = 0;
    size_t key_len = strlen(key);

    for (; i < key_len; i++)
    {
        value += value * 47 + key[i];
    }
    value = value % map->len;
    return value;
}

int hashmap_insert_node(HASHMAP *map, char* key, void *value)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return -1;

    HASHNODE *node = hashnode_init();
    if(node == NULL) return -1;
    int key_len = strlen(key) + 1;
    node->key = (char *)malloc(key_len);
    if(node->key == NULL)
    {
        free(node);
        return -1;
    }
    strcpy(node->key, key);
    node->value = value;

    if(map->table[index] == NULL)
    {
        map->table[index] = node;
        return index;
    }

    HASHNODE *curr = map->table[index];
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    curr->next = node;
    return index;
}


int hashmap_rm_node(HASHMAP *map, char* key)
{
    if(map == NULL || key == NULL) return -1;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return -1;

    HASHNODE *prev = NULL;
    HASHNODE *node = map->table[index];

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

    hashnode_del(node);
    return 0;
}

HASHNODE *hashmap_find_node(HASHMAP *map, char* key)
{
    if(map == NULL || key == NULL) return NULL;
    unsigned int index = hash_gen_key(map, key);
    if(index < 0 || index > map->len) return NULL;

    HASHNODE *node = map->table[index];
    if(strcmp(node->key, key) == 0) return node;

    while (node->next != NULL)
    {
        node = node->next;
        if(strcmp(node->key, key) == 0) return node;
    }
    return NULL;
}

