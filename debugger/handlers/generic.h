#ifndef CDB_HANDLERS_GENERIC
#define CDB_HANDLERS_GENERIC

#include "json.h"
#include "sys/types.h"

/*
    resp = {
        "pid":          pid,        - pid for which the action was called
        "action_id":    aid,        - action handler id
        "result":       json result - return value = { data: null|some-value }
    }
*/

int get_regs();
int no_action();
int attach_process(JSON *args);

#endif
