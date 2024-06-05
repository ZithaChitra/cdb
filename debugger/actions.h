#ifndef CDB_ACTIONS
#define CDB_ACTIONS

#include "cjson/cJSON.h"
#include "json.h"
#include "cdb.h"

/*
    resp = {
        "pid":      pid,        - pid for which the action was called
        "actid":    aid,        - action handler id
        "result":   json result - return value = { data: null|some-value }
    }

    action = {
        "pid":      pid,        - pid for which the action was called
        "actid":    aid,        - action handler id
        "args":     json args   - action handler arguments
    }
*/

typedef int (*HANDLER_CDB)(CDB *cdb, JSON *args, char **resp_str);

typedef enum cdb_actions{
    PROC_ATTACH = 0,        // - 0
    PROC_DETACH,            // - 1
    PROC_GET_REGS,          // - 2
    UNKNOWN_ACTION,         // - 3
} CDB_ACTIONS;

#endif