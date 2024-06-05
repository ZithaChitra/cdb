#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>
#include <stdlib.h>
#include "generic.h"
#include "cdb.h"
#include "json.h"
#include "trace.h"


JSON *init_handler_resp(pid_t *pid)
{
    JSON *resp = json_init("empty", NULL);
    json_add_val(resp, "number", "pid", pid);
    json_add_val(resp, "string", "status", "succuess");
    return resp;
}
int proc_detach(CDB *cdb, JSON *args, char **resp_str)
{
    printf("\n\nattach_process handler start\n");

    if(cdb == NULL)
    {
        printf("cdb not provided. could not attach\n");
        return -1;
    }

    JSON *pid = json_get_value(args, "pid");
    if (pid == NULL)
    {
        printf("process id not provided. could not attach\n");
        return -1;
    }


    int proc_exists = cdb_has_proc(cdb, pid->valueint);
    if(proc_exists == 1)
    {
        printf("process already attached\n");
        if(_trace_detach(pid->valueint) == -1) return -1;
        if(cdb_remove_proc(cdb, pid->valueint, -1) == -1) return -1;
        return -1;
    }

    printf("process not found on cdb\n");
    return -1;
}

int proc_attach(CDB *cdb, JSON *args, char **resp_str)
{
    /**
     * pid: pid of process to attach
    */
    printf("\n\nattach_process handler start\n");

    if(cdb == NULL)
    {
        printf("cdb not provided. could not attach\n");
        return -1;
    }

    JSON *pid = json_get_value(args, "pid");
    if (pid == NULL)
    {
        printf("process id not provided. could not attach\n");
        return -1;
    }

    int proc_exists = cdb_has_proc(cdb, pid->valueint);
    if(proc_exists)
    {
        printf("process already attached\n");
        return -1;
    }

    if(_trace_attach(pid->valueint) == -1) return -1;
    
    PROCESS *proc = proc_init(pid->valueint);
    if(proc == NULL)
    {
        printf("could not allocate memory for new process\n");
        _trace_detach(pid->valueint);
        return -1;
    }

    int status = cdb_add_proc(cdb, proc);
    if(status == -1)
    {
        printf("cdb currently as max proc capacity\n");
        _trace_detach(pid->valueint);
        proc_delete(proc);
        return -1;
    }

    printf("attach_process handler finish\n");
    printf("---------------------\n\n");

    return 0;
}

int proc_get_regs(CDB *cdb, JSON *args, char **resp_str)
{
    printf("-------------------------------\n");
    printf("generic: action handler: get_regs\n");
    if(cdb == NULL || args == NULL) return -1;
    JSON *pid = json_get_value(args, "pid");
    if (pid == NULL)
    {
        printf("process id not provided. no regs\n");
        return -1;
    }
    int proc_exists = cdb_has_proc(cdb, pid->valueint);
    if(proc_exists)
    {
        struct user_regs_struct *regs = _trace_proc_get_regs(pid->valueint);
        if(regs == NULL) return -1;
        JSON *resp = json_init("empty", NULL);
        if(resp == NULL)
        {
            printf("could not allocate memory for response\n");
            free(regs);
            return 1;
        }
        cJSON_AddNumberToObject((cJSON *)resp, "rip", regs->rip);
        cJSON_AddNumberToObject((cJSON *)resp, "rsp", regs->rsp);
        cJSON_AddNumberToObject((cJSON *)resp, "rbp", regs->rbp);
        cJSON_AddNumberToObject((cJSON *)resp, "rax", regs->rax);
        cJSON_AddNumberToObject((cJSON *)resp, "rbx", regs->rbx);
        cJSON_AddNumberToObject((cJSON *)resp, "rcx", regs->rcx);
        cJSON_AddNumberToObject((cJSON *)resp, "rdx", regs->rdx);
        cJSON_AddNumberToObject((cJSON *)resp, "rsi", regs->rsi);
        cJSON_AddNumberToObject((cJSON *)resp, "rdi", regs->rdi);
        cJSON_AddNumberToObject((cJSON *)resp, "r8",  regs->r8);
        cJSON_AddNumberToObject((cJSON *)resp, "r9",  regs->r9);
        cJSON_AddNumberToObject((cJSON *)resp, "r10", regs->r10);
        cJSON_AddNumberToObject((cJSON *)resp, "r11", regs->r11);
        cJSON_AddNumberToObject((cJSON *)resp, "r12", regs->r12);
        cJSON_AddNumberToObject((cJSON *)resp, "r13", regs->r13);
        cJSON_AddNumberToObject((cJSON *)resp, "r14", regs->r14);
        cJSON_AddNumberToObject((cJSON *)resp, "r15", regs->r15);

        JSON *root = json_init("empty", NULL);
        if(root == NULL)
        {
            printf("could not allocate memory for response\n");
            free(regs);
            json_delete(resp);
            return -1;
        }

        cJSON_AddItemToObject((cJSON *)root, "regs", resp);
        *resp_str = cJSON_PrintUnformatted(root);
        printf("%s\n", *resp_str);
        printf("-------------------------------\n\n");
        
        json_delete(root);
        free(regs);
        return 0;
    }
    printf("-------------------------------\n\n");
    return -1;
}

int no_action()
{
    return 0;
}

