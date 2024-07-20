#include "actions.h"
#include "handlers/generic.h"

HANDLER_CDB action_handlers[] = {
    (HANDLER_CDB)proc_start_dbg,        //  - 0
    (HANDLER_CDB)proc_end_dbg,          //  - 1 
    (HANDLER_CDB)proc_regs_read,        //  - 2
    (HANDLER_CDB)proc_regs_write,       //  - 3
    (HANDLER_CDB)proc_mem_read,         //  - 4
    (HANDLER_CDB)proc_mem_write,        //  - 5
    (HANDLER_CDB)proc_step_single,      //  - 6
    (HANDLER_CDB)proc_func_all,         //  - 7
    (HANDLER_CDB)proc_func_single,      //  - 8
    (HANDLER_CDB)proc_break,            //  - 9
    (HANDLER_CDB)proc_cont,             //  - 10
    (HANDLER_CDB)proc_stdout,           //  - 11
    (HANDLER_CDB)proc_stdin,            //  - 12
    (HANDLER_CDB)file_get,              //  - 13
    (HANDLER_CDB)no_action,             //  - 11

};
