#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#include "actions.h"
#include "trace.h"
#include "cdb.h"

extern HANDLER_CDB action_handlers[];
extern CDB *cdb_main;

PROCESS *proc_init(pid_t pid)
{
    PROCESS *proc = (PROCESS *)malloc(sizeof(PROCESS));
    if(proc == NULL) return NULL;
    proc->pid       = pid;
    proc->src       = NULL;
    proc->exec_addr = _trace_find_exec_addr(pid);
    proc->dw_dbg    = 0;
    return proc;
}

void proc_delete(PROCESS *proc)
{
    if(proc == NULL) return;
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
    free(proc);
}

CDB *cdb_init()
{
    CDB *cdb = (CDB *)malloc(sizeof(CDB)); 
    if(cdb == NULL) return NULL;
    cdb->proc_curs = 0;
    return cdb;
}

void cdb_delete()
{
    printf("------cdb_delete start------------------\n");
    if(cdb_main == NULL) return;
    for (size_t i = 0; i < cdb_main->proc_curs; i++)
    {
        PROCESS *proc = cdb_main->all_proc[i];
        if(proc == NULL) continue;
        printf("removing pid: %d\n", proc->pid);
        if (_trace_is_proc_attached(proc->pid))
        {
           _trace_proc_cont_kill(proc->pid);
        }
        proc_delete(proc);
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
    for (size_t i = 0; i < cdb->proc_curs; i++)
    {
        PROCESS *proc = cdb->all_proc[i];
        if(proc == NULL) continue;
        if(proc->pid == pid) return 1;
    }
    return 0;
}

PROCESS *cdb_find_proc(CDB *cdb, pid_t pid)
{
    if(cdb == NULL) return NULL;
    PROCESS *proc = NULL;
    for (size_t i = 0; i < cdb->proc_curs; i++)
    {
        proc = cdb->all_proc[i];
        if(proc == NULL) continue;
        if(proc->pid == pid) return proc;
    }
    return NULL;
}

int cdb_add_proc(CDB *cdb, PROCESS *proc)
{
    if(cdb->proc_curs < MAX_PROC)
    {
        cdb->all_proc[cdb->proc_curs] = proc;
        cdb->proc_curs++;
        return 0;
    }
    return -1;
}

int cdb_remove_proc(CDB *cdb, pid_t pid, int index)
{
    if(cdb  == NULL) return -1;
    PROCESS *proc;
    if(index == -1) // we dont know the proc index
    {
        for (size_t i = 0; i < cdb->proc_curs; i++)
        {
            proc = cdb->all_proc[i];
            if(proc == NULL) continue;
            if(proc->pid == pid) proc_delete(proc);
        }
        return 0;
    }

    proc = cdb->all_proc[index];
    if(proc == NULL) return -1;
    proc_delete(proc);
    return 0;
}