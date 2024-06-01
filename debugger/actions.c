#include "actions.h"
#include "handlers/generic.h"

enum cdb_actions {
    GET_REGS = 1,
    UNKNOWN,
};

// void get_regs()
// {
//     printf("-------------------------------\n");
//     printf("action handler: get_regs\n");
//     printf("-------------------------------\n");
//     return;
// }

HANDLER_CDB action_handlers[] = {
    (HANDLER_CDB)get_regs,
    // (HANDLER_CDB)no_action,
};

