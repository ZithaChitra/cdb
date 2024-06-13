#include "actions.h"
#include "handlers/generic.h"

HANDLER_CDB action_handlers[] = {
    (HANDLER_CDB)proc_start_dbg,
    (HANDLER_CDB)proc_end_dbg,
    (HANDLER_CDB)proc_regs_read,
    (HANDLER_CDB)proc_regs_write,
    (HANDLER_CDB)proc_mem_read,
    (HANDLER_CDB)proc_mem_write,
    (HANDLER_CDB)proc_step_single,
    (HANDLER_CDB)no_action,
};
