#ifndef CDB_HASHMAP
#define CDB_HASHMAP

#define MAP_LEN 5

typedef struct HASH_NODE HASH_NODE;
typedef struct HASHMAP HASHMAP;

struct HASH_NODE 
{
    char *key;
    void *value;
    HASH_NODE *next;
};

struct HASHMAP
{
    int len;
    HASH_NODE **table;
};

HASHMAP* hashmap_init();
HASH_NODE *hash_node_init();
unsigned int hash_gen_key(HASHMAP *map, char *key);
int hashmap_insert(HASHMAP *map, char* key, void *val);
int hashmap_delete(HASHMAP *map, char* key);
HASH_NODE *hashmap_find(HASHMAP *map, char* key);

#endif