#ifndef CDB_DEBUGGER
#define CDB_DEBUGGER

#include <sys/types.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include "json.h"
#include "data/hashmap.h"

#define MAX_PROC 1
#define ADDR_LEN 100

typedef struct process
{
    pid_t pid;
    FILE *src;
    void *exec_addr; // text addr
    Dwarf_Debug dw_dbg;
    HASHMAP *breaks;
} PROCESS;


typedef struct breakp
{
    void *addr;
    long og_code;
} BREAKP;

typedef struct cdb
{
    int proc_curs;
    PROCESS *all_proc[MAX_PROC];
} CDB;

PROCESS *proc_init(pid_t pid);
void proc_delete(PROCESS *proc);
int proc_add_breakp(PROCESS *proc, void **addr, long og_code);
BREAKP *proc_find_breakp(PROCESS *proc, void *addr); // find saved bp
int proc_rm_break(PROCESS *proc, char *addr);

BREAKP *breakp_init(void *addr, long og_code);
void breakp_delete(BREAKP *bp);

CDB *cdb_init();
int cdb_has_proc(CDB *cdb, pid_t pid);
PROCESS *cdb_find_proc(CDB *cdb, pid_t pid);
int cdb_add_proc(CDB *cdb, PROCESS *proc);
int cdb_remove_proc(CDB *cdb, pid_t pid, int index);
JSON *cdb_exec_action(CDB *cdb, JSON* action, char **resp_str);
void cdb_delete();

#endif