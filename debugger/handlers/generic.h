#ifndef CDB_HANDLERS_GENERIC
#define CDB_HANDLERS_GENERIC

#include <sys/types.h>
#include "json.h"
#include "cdb.h"

int proc_start_dbg(CDB *cdb, JSON *args, char **resp_str);
int proc_end_dbg(CDB *cdb, JSON *args, char **resp_str);
int proc_regs_read(CDB *cdb, JSON *args, char **resp_str);
int proc_regs_write(CDB *cdb, JSON *args, char **resp_str);
int proc_mem_read(CDB *cdb, JSON *args, char **resp_str);
int proc_mem_write(CDB *cdb, JSON *args, char **resp_str);
int proc_step_single(CDB *cdb, JSON *args, char **resp_str);
int no_action();

#endif
