#ifndef CDB_DEBUGGER
#define CDB_DEBUGGER

#include <sys/types.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include <sys/user.h>
#include "json.h"
#include "data/hashmap.h"
#include "data/list.h"
#include "dwarf/resolve.h"

#define MAX_PROC        1
#define ADDR_LEN        100
#define PROC_BUFF_SIZE  2024

typedef struct procstate
{
    LINEINFO line_info;
    struct user_regs_struct regs;
} PROCSTATE;

typedef struct process
{
    pid_t       pid;
    int         ws_fd;          // the connected fd that started this process
    FILE        *src;
    void        *base_addr;     //  base address where proc is loaded in memory
    LIST        list_node;      //  entry in CDB process list
    void        *proc_buffer;   //  buffer for IO
    HASHMAP     *breaks;        //  break points
    PROCSTATE   curr_state;
    Dwarf_Debug dw_dbg;         
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
    LIST *all_procs;


} CDB;

PROCESS *proc_init(pid_t pid, int ws_fd);
void proc_delete(PROCESS *proc);
int proc_add_breakp(PROCESS *proc, void **addr, long og_code);
BREAKP *proc_find_breakp(PROCESS *proc, void *addr); // find saved bp
int proc_rm_break(PROCESS *proc, char *addr);
int proc_get_curr_state(PROCESS *proc, struct user_regs_struct *regs);
JSON *proc_state_to_json(PROCESS *proc);
int proc_init_buffer();
int proc_clear_buffer();
int proc_delete_buffer();

BREAKP *breakp_init(void *addr, long og_code);
void breakp_delete(BREAKP *bp);

CDB *cdb_init();
int cdb_has_proc(CDB *cdb, pid_t pid);
PROCESS *cdb_find_proc(CDB *cdb, pid_t pid);
int cdb_add_proc(CDB *cdb, PROCESS *proc);
int cdb_remove_proc(CDB *cdb, pid_t pid);
JSON *cdb_exec_action(CDB *cdb, JSON* action, char **resp_str, int ws_fd);
void cdb_rm_conn_procs(CDB *cdb, int conn_fd); // stop debugging procs belonging to a connection
void cdb_delete();

#endif

