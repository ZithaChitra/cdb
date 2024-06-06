#include "actions.h"
#include "handlers/generic.h"

HANDLER_CDB action_handlers[] = {
    (HANDLER_CDB)proc_attach,
    (HANDLER_CDB)proc_detach,
    (HANDLER_CDB)proc_get_regs,
    (HANDLER_CDB)proc_mem_read,
    (HANDLER_CDB)proc_mem_write,
    (HANDLER_CDB)no_action,
};
