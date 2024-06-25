#ifndef CDB_HASHMAP
#define CDB_HASHMAP

#define MAP_LEN 5

typedef struct HASHNODE HASHNODE;
typedef struct HASHMAP HASHMAP;

struct HASHNODE 
{
    char *key;
    void *value;
    HASHNODE *next;
};

struct HASHMAP
{
    int len;
    HASHNODE **table;
};

HASHMAP* hashmap_init();
HASHNODE *hashnode_init();
unsigned int hash_gen_key(HASHMAP *map, char *key);
int hashmap_insert_node(HASHMAP *map, char* key, void *val);
int hashmap_rm_node(HASHMAP *map, char* key);
HASHNODE *hashmap_find_node(HASHMAP *map, char* key);
int hashnode_del(HASHNODE *node);

#endif