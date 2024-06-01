#include "json.h"
#include "string.h"
#include "cjson/cJSON.h"
#include "stdio.h"
#include "stdlib.h"

/*
    - Wrapper for cJSON
    - https://github.com/DaveGamble/cJSON
*/

/* 
char *json_types = {
    "empty", "null", "bool", "true", "false", "number", "string"
    };
 */
static char *json_types[] = {
    "empty", "null", "bool", "true", 
    "false", "number", "string"
    };

void json_print(JSON *obj)
{
    if(obj == NULL) return;
    char *str = cJSON_Print(obj);
    if(str == NULL)
    {
        printf("could not print json object\n");
        return;
    }
    printf("%s\n", str);
    free(str);
    return;
}

void json_delete(JSON *obj)
{
    if(obj == NULL) return;
    cJSON_Delete(obj);
    return;
}


JSON *json_init(char *type, void *value)
{
    JSON *obj;
    if(strcmp(type, "empty") == 0)
    {
        obj = cJSON_CreateObject(); 
        if (obj) return obj;
    }
    else if(strcmp(type, "string") == 0)
    {
        if(value == NULL) return NULL;
        obj = cJSON_CreateString((char *)value);
        return obj;
    }
    else if(strcmp(type, "number") == 0)
    {
        if(value == NULL) return NULL;
        obj = cJSON_CreateNumber(*(int *)value);
        return obj;
    }
    return NULL;
}

int json_add_val(JSON *obj, char *type, char *key, void *value)
{
    if(obj == NULL || type == NULL || key == NULL || value == NULL) return -1;
    JSON *val;
    if(strcmp(type, "string") == 0)
    {
        val = json_init("string", value);
        if(val == NULL) return -1;
        cJSON_AddItemToObject(obj, key, val);
        return 0;
    }
    else if(strcmp(type, "number") == 0)
    {
        val = json_init("number", value);
        if(val == NULL) return -1;
        cJSON_AddItemToObject(obj, key, val);
        return 0;
    }
    return -1;
}

JSON *json_from_string(char *json_string)
{
    JSON *obj = cJSON_Parse(json_string);
    return obj;
}

JSON *json_get_value(JSON *obj, char *key)
{
    if(obj == NULL || key == NULL) return NULL;
    JSON *value = cJSON_GetObjectItemCaseSensitive(obj, key);
    return value;
}

// int json_value_is_string(JSON *obj)
// {
//     return cJSON_IsString(obj);
// }

// int json_value_is_number(JSON *obj);
