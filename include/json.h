#ifndef CDB_JSON
#define CDB_JSON


/*
    - Wrapper for cJSON
    - https://github.com/DaveGamble/cJSON
*/

#include "cjson/cJSON.h"

typedef cJSON JSON;


JSON *json_init(char *type, void *value);
JSON *json_from_string(char *json_string);
JSON *json_get_value(JSON *obj, char *key);
int json_add_val(JSON *obj, char *type, char *key, void *value);

void json_print(JSON *obj);
void json_delete(JSON *obj);

int json_value_is_string(JSON *obj);
int json_value_is_number(JSON *obj);

#endif