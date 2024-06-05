#ifndef CDB_DEBUGGER
#define CDB_DEBUGGER

#include "sys/types.h"
#include "json.h"

#define MAX_PROC 1

typedef struct process
{
    pid_t pid;
} PROCESS;

typedef struct cdb
{
    int proc_curs;
    PROCESS *all_proc[MAX_PROC];
} CDB;

PROCESS *proc_init(pid_t pid);
void proc_delete(PROCESS *proc);

CDB *cdb_init();
int cdb_has_proc(CDB *cdb, pid_t pid);
int cdb_add_proc(CDB *cdb, PROCESS *proc);
int cdb_remove_proc(CDB *cdb, pid_t pid, int index);
JSON *cdb_exec_action(CDB *cdb, JSON* action, char **resp_str);
void cdb_delete();

#endif