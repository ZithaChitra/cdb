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
    PROC_START_DBG = 0,         // - 0  : start debugging
    PROC_END_DBG,               // - 1  : stop debugging
    PROC_REGS_READ,             // - 2  : read all registers
    PROC_REGS_WRITE,            // - 3  : modify a register
    PROC_MEM_READ,              // - 4  : read from memory
    PROC_MEM_WRITE,             // - 5  : write memory
    PROC_STEP_SINGLE,           // - 6  : exec single instruction 
    PROC_FUNC_ALL,              // - 7  : all functions
    PROC_FUNC_SINGLE,           // - 8  : single function
    PROC_BREAK,                 // - 9  : set software breakpoint
    PROC_CONT,                  // - 10 : continue exec
    PROC_STDOUT,                // - 11 : data from tracee stdout
    PROC_STDIN,                 // - 13 : data to tracee stdin
    UNKNOWN_ACTION,             // - 14
} CDB_ACTIONS;

#endif