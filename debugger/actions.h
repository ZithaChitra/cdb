#ifndef CDB_ACTIONS
#define CDB_ACTIONS

#include "cjson/cJSON.h"
#include "json.h"

typedef int (*HANDLER_CDB)(JSON *args);

typedef enum cdb_actions{
    ATTACH_PROCESS = 0,     // - 0
    GET_REGS,               // - 1
    UNKNOWN_ACTION,         // - 2
} CDB_ACTIONS;


#endif