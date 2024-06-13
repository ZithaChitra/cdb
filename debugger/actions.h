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
    PROC_START_DBG = 0,         // - 0
    PROC_END_DBG,               // - 1
    PROC_REGS_READ,             // - 2
    PROC_REGS_WRITE,            // - 3
    PROC_MEM_READ,              // - 4
    PROC_MEM_WRITE,             // - 5
    PROC_STEP_SINGLE,           // - 6
    UNKNOWN_ACTION,             // - 7
} CDB_ACTIONS;

#endif