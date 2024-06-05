#ifndef CDB_HANDLERS_GENERIC
#define CDB_HANDLERS_GENERIC

#include <sys/types.h>
#include "json.h"
#include "cdb.h"

int proc_attach(CDB *cdb, JSON *args, char **resp_str);
int proc_detach(CDB *cdb, JSON *args, char **resp_str);
int proc_get_regs(CDB *cdb, JSON *args, char **resp_str);
int no_action();

#endif
