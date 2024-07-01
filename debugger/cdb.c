#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include "actions.h"
#include "trace.h"
#include "cdb.h"
#include "data/hashmap.h"
#include "data/list.h"


extern HANDLER_CDB action_handlers[];
extern CDB *cdb_main;

PROCESS *proc_init(pid_t pid)
{
    PROCESS *proc = (PROCESS *)malloc(sizeof(PROCESS));
    if(proc == NULL) return NULL;
    proc->breaks    = hashmap_init();
    if(proc->breaks == NULL)
    {
        free(proc);
        return NULL;
    }
    proc->list_node = list_init();
    if(proc->list_node == NULL)
    {
        free(proc->breaks);
        free(proc);
        return NULL;
    }
    proc->pid       = pid;
    proc->src       = NULL;
    proc->exec_addr = _trace_find_exec_addr(pid);
    proc->dw_dbg    = 0;
    return proc;
}

void proc_delete(PROCESS *proc)
{
    if(proc == NULL) return;
    // list_rm_node(proc->list_node);
    if(proc->dw_dbg)
    {
        Dwarf_Error err;
        if(dwarf_finish(proc->dw_dbg, &err) != DW_DLV_OK)
        {
            fprintf(stderr, "proc_delete: error in dwarf finidh: %s\n",
                dwarf_errmsg(err));
            dwarf_dealloc(proc->dw_dbg, err, DW_DLA_ERROR);
        }
    }
    if(proc->src) fclose(proc->src);
    // free breaks hashamp
    free(proc);
}

BREAKP *breakp_init(void *addr, long og_code)
{
    BREAKP *breakp = (BREAKP *)malloc(sizeof(BREAKP));
    if(breakp == NULL) return NULL;
    breakp->addr    = addr;
    breakp->og_code = og_code;
    return breakp;
}

void breakp_delete(BREAKP *bp)
{
    if(bp == NULL) return;
    free(bp);
}

BREAKP *proc_find_breakp(PROCESS *proc, void *addr) // find saved bp
{
    if(proc == NULL || addr == NULL) return NULL;
    char addr_str[ADDR_LEN];
    snprintf(addr_str, ADDR_LEN, "%p", addr);
    printf("searching for breakpoint at addr: %s\n", addr_str);
    HASHNODE *bpn = hashmap_find_node(proc->breaks, addr_str);
    
    if(bpn == NULL) return NULL;
    return (BREAKP *)bpn->value;
}


int proc_add_breakp(PROCESS *proc, void **addr, long og_code)
{
    if(proc == NULL || addr == NULL) return -1;
    char addr_str[ADDR_LEN];
    snprintf(addr_str, ADDR_LEN, "%p", *addr);

    char *key = (char *)malloc((ADDR_LEN + 1) * sizeof(char));
    if(key == NULL) return -1;
    strcpy(key, addr_str);

    BREAKP *bp = breakp_init(*addr, og_code);
    if(bp == NULL)
    {
        free(key);
        return -1;
    }
    return hashmap_insert_node(proc->breaks, key, (void *)bp);
}

int proc_rm_break(PROCESS *proc, char *addr)
{
    if(proc == NULL || addr == NULL) return -1;
    return hashmap_rm_node(proc->breaks, addr);
}


CDB *cdb_init()
{
    CDB *cdb = (CDB *)malloc(sizeof(CDB)); 
    if(cdb == NULL) return NULL;
    cdb->proc_curs = 0;
    cdb->all_procs = list_init();
    if(cdb->all_procs == NULL)
    {
        free(cdb);
        return NULL;
    }
    return cdb;
}

void cdb_delete()
{
    printf("------cdb_delete start------------------\n");
    if(cdb_main == NULL) return;
    PROCESS *proc = NULL;
    LIST *curs    = NULL;
    LIST_FOR_EACH(curs, cdb_main->all_procs)
    {
        proc = LIST_PARENT(curs, PROCESS, list_node);
        if(proc == NULL) continue;
        printf("removing pid: %d\n", proc->pid);
        if (_trace_is_proc_attached(proc->pid))
        {
           _trace_proc_cont_kill(proc->pid);
        }
        proc_delete(proc);
    }
    LIST *curr = NULL;
    for(curs = cdb_main->all_procs; curs != NULL;)
    {
        curr = curs;
        curs = curs->next;
        list_rm_node(curr);
    }
    free(cdb_main);
    printf("------cdb_delete end------------------\n");
    exit(EXIT_SUCCESS);
} 


JSON *cdb_exec_action(CDB *cdb, JSON *action, char **resp_str)
{
    /*
        most importantly is that an action should 
        have an `actid` property which will index 
        into the debugger's action handlers.  
    */

    if (cdb == NULL)
    {
        printf("cdb: debugger not initialized\n");
        return NULL;
    }

    if (action == NULL) {
        printf("Error parsing JSON\n");
        return NULL;
    }
    else{
        printf("Success parsing JSON\n");
    }
    JSON *actid = json_get_value(action, "actid");
    JSON *args  = json_get_value(action, "args");

    if(actid == NULL)
    {
        printf("actid not privided in JSON\n");
        return NULL;
    }

    int action_id = actid->valueint;
    if(action_id < UNKNOWN_ACTION)
    {
        cJSON_AddNumberToObject(args, "actid", action_id);
        printf("action id: %d\n", action_id);
        HANDLER_CDB handler = action_handlers[action_id];
        handler(cdb, args, resp_str);
    }
    else
    {
        printf("action not recognized: %d\n", action_id);
    }
    json_print(action);
    return action; // should actully return resp frm handler
}

int cdb_has_proc(CDB *cdb, pid_t pid)
{
    if (cdb == NULL) return -1;
    PROCESS *proc = NULL;
    LIST *curs    = NULL;
    LIST_FOR_EACH(curs, cdb->all_procs)
    {
        proc = LIST_PARENT(curs, PROCESS, list_node);
        if(proc == NULL) continue;
        if(proc->pid == pid) return 1;
    }
    return 0;
}

PROCESS *cdb_find_proc(CDB *cdb, pid_t pid)
{
    if(cdb == NULL) return NULL;
    PROCESS *proc = NULL;
    LIST *curs    = NULL;
    LIST_FOR_EACH(curs, cdb->all_procs)
    {
        proc = LIST_PARENT(curs, PROCESS, list_node);
        if(proc != NULL) if(proc->pid == pid) return proc;
    }
    return NULL;
}

int cdb_add_proc(CDB *cdb, PROCESS *proc)
{
    LIST *curs = NULL;
    for (curs = cdb->all_procs; curs != NULL; curs = curs->next)
    {
        if(curs->next == NULL) 
        {
            curs->next = proc->list_node;
            return 0;
        }
    }
    return -1;
}

int cdb_remove_proc(CDB *cdb, pid_t pid, int index)
{
    if(cdb == NULL) return -1;
    PROCESS *proc = NULL;
    LIST *curs    = NULL;
    LIST_FOR_EACH(curs, cdb->all_procs)
    {
        proc = LIST_PARENT(curs, PROCESS, list_node);
        if(proc != NULL && proc->pid == pid)
        {
            list_rm_node(curs);
            return 0;
        }
    }
    proc_delete(proc);
    return 0;
}