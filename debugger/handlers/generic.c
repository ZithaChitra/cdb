#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>
#include <string.h>
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

int proc_start_dbg(CDB *cdb, JSON *args, char **resp_str)
{
    printf("handler: proc_start_debug");
    JSON *procpath = json_get_value(args, "path");
    printf("proc path: %s\n", procpath->valuestring);
    pid_t pid = _trace_proc_start(procpath->valuestring);
    printf("started process: %d\n", pid);
    if(pid == -1)
    {
        return -1;
    }
    int pathlen = strlen(procpath->valuestring);
    char *path = (char *)malloc(pathlen);
    if(path == NULL) return -1;
    PROCESS *proc = proc_init(pid);
    if(proc == NULL)
    {
        printf("could not allocate memory for new process\n");
        _trace_proc_cont_kill(pid);
        return -1;
    }

    int status = cdb_add_proc(cdb, proc);
    if(status == -1)
    {
        printf("cdb currently as max proc capacity\n");
        _trace_proc_cont_kill(pid);
        proc_delete(proc);
        return -1;
    }
    JSON *resp = json_init("empty", NULL);
    if(resp == NULL)
    {
        printf("could not allocate memory for response\n");
        return -1;
    }
    
    JSON *resp_ = json_init("empty", NULL);
    if(resp == NULL)
    {
        printf("could not allocate memory for response\n");
        return -1;
    }
    JSON *action = json_get_value(args, "actid");
    cJSON_AddNumberToObject(resp, "actid", action->valueint);
    cJSON_AddNumberToObject(resp_, "pid", pid);
    cJSON_AddStringToObject(resp_, "path", procpath->valuestring);
    cJSON_AddItemToObject(resp, "resp", resp_);
    *resp_str = cJSON_PrintUnformatted(resp);
    printf("debugging proc: %s\n", procpath->valuestring);
    json_delete(resp);
    return 0;
}

int proc_end_dbg(CDB *cdb, JSON *args, char **resp_str)
{

    printf("\n\nproc_end handler start\n");

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
        if(_trace_proc_cont_kill(pid->valueint) == -1) return -1;
        if(cdb_remove_proc(cdb, pid->valueint, -1) == -1) return -1;
        return -1;
    }

    printf("process not found on cdb\n");
    return -1;
}


int proc_regs_read(CDB *cdb, JSON *args, char **resp_str)
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
        JSON *regs_json = json_init("empty", NULL);
        if(regs_json == NULL)
        {
            printf("could not allocate memory for response\n");
            free(regs);
            json_delete(resp);
            return 1;
        }
        JSON *resp_ = json_init("empty", NULL);
        if(resp_ == NULL)
        {
            printf("could not allocate memory for response\n");
            free(regs);
            json_delete(resp);
            json_delete(regs_json);
            return 1;
        }
        cJSON_AddNumberToObject((cJSON *)regs_json, "rip", regs->rip);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rsp", regs->rsp);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rbp", regs->rbp);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rax", regs->rax);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rbx", regs->rbx);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rcx", regs->rcx);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rdx", regs->rdx);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rsi", regs->rsi);
        cJSON_AddNumberToObject((cJSON *)regs_json, "rdi", regs->rdi);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r8",  regs->r8);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r9",  regs->r9);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r10", regs->r10);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r11", regs->r11);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r12", regs->r12);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r13", regs->r13);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r14", regs->r14);
        cJSON_AddNumberToObject((cJSON *)regs_json, "r15", regs->r15);

        JSON *action = json_get_value(args, "actid");
        cJSON_AddNumberToObject(resp, "actid", action->valueint);

        cJSON_AddNumberToObject(resp_, "pid", pid->valueint);
        cJSON_AddItemToObject((cJSON *)resp_, "regs", regs_json);

        cJSON_AddItemToObject(resp, "resp", resp_);

        // cJSON_AddIte
        *resp_str = cJSON_PrintUnformatted(resp);
        printf("%s\n", *resp_str);
        printf("-------------------------------\n\n");
        
        json_delete(resp);
        free(regs);
        return 0;
    }
    printf("-------------------------------\n\n");
    return -1;
}

int proc_regs_write(CDB *cdb, JSON *args, char **resp_str)
{

}

int proc_mem_read(CDB *cdb, JSON *args, char **resp_str)
{
    
    printf("-------------------------------\n");
    printf("generic: action handler: proc_mem_read\n");
    if (cdb  == NULL || args  == NULL) return -1;
    JSON *pid = json_get_value(args, "pid");
    if (pid == NULL)
    {
        printf("process id not provided. no regs\n");
        return -1;
    }
    int proc_exists = cdb_has_proc(cdb, pid->valueint);
    if(!proc_exists) return -1;
    long read_size = 5;
    unsigned long mem_buffer[5024];
    unsigned long long exec_addr = _trace_find_exec_addr(pid->valueint);
    _trace_proc_mem_read(pid->valueint, exec_addr, mem_buffer, read_size);
    char str_val[20];
    char *resp = (char *)malloc(read_size * (sizeof(char) * 20)); 
    if(resp == NULL) return -1;
    resp[0] = '\0';

    for (size_t i = 0; i < read_size; i++)
    {
        fprintf(stdout, "value: %lx\n", *(mem_buffer + (i * sizeof(unsigned long))));
        snprintf(str_val, sizeof(str_val), "0x%lx,", *(mem_buffer + (i * sizeof(unsigned long))));
        strcat(resp, str_val);
    }

    printf("%s\n", resp);
    *resp_str = resp;

    // // Calculate the required size for the final string
    // size_t total_length = 1; // Start with 1 for the null terminator
    // for (size_t i = 0; i < read_size; ++i) {
    //     total_length += snprintf(NULL, 0, "%lx", mem_buffer[i]) + 1; // +1 for the comma or null terminator
    // }

    // // Allocate memory for the final string
    // char *final_str = malloc(total_length);
    // if (final_str == NULL) {
    //     fprintf(stderr, "Memory allocation failed.\n");
    //     return EXIT_FAILURE;
    // }

    // // Build the final comma-separated string
    // final_str[0] = '\0'; // Initialize the final string
    // for (size_t i = 0; i < read_size; ++i) {
    //     char buffer[20];
    //     snprintf(buffer, sizeof(buffer), "%lx", mem_buffer[i]);
    //     strcat(final_str, buffer);
    //     if (i < read_size - 1) {
    //         strcat(final_str, ",");
    //     }
    // }
    // *resp_str = final_str;
    
    printf("generic: action handler: proc_mem_read\n");
    printf("-------------------------------\n\n");
    return 0;
}

int proc_mem_write(CDB *cdb, JSON *args, char **resp_str)
{
    if (cdb  == NULL || args  == NULL) return -1;
    return 0;
}

int proc_step_single(CDB *cdb, JSON *args, char **resp_str)
{
    printf("handler: proc_step_single");
    JSON *pid = json_get_value(args, "pid");
    if(pid == NULL) return -1;
    if (_trace_proc_step_single(pid->valueint) == -1) return -1;
    printf("ran single step: calling - proc_regs_read");
    return proc_regs_read(cdb, args, resp_str);
}


int no_action()
{
    return 0;
}