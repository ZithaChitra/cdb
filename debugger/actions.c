#include "actions.h"
#include "handlers/generic.h"


HANDLER_CDB action_handlers[] = {
    (HANDLER_CDB)attach_process,
    (HANDLER_CDB)get_regs,
    (HANDLER_CDB)no_action,
};
