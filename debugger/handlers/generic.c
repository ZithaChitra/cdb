#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>
#include <string.h>
#include <stdlib.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <sys/uio.h>
#include "generic.h"
#include "cdb.h"
#include "json.h"
#include "trace.h"
#include "resolve.h"


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
    if(pid == -1) return -1;

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

    proc->src = fopen("demo", "rb");
    if(proc->src == NULL)
    {
        _trace_proc_cont_kill(pid);
        proc_delete(proc);
        printf("could not open debug file\n");
        return -1;
    }else{
        printf("openned file\n");
    }

    Dwarf_Error err;
    if(dwarf_init(fileno(proc->src), DW_DLC_READ, NULL, 
        NULL, &proc->dw_dbg, &err) != DW_DLV_OK)
    {
        _trace_proc_cont_kill(pid);
        proc_delete(proc);
        printf("could not init debug info\n");
        return -1;
    }else{
        printf("initiated debug info\n");
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
    return 0;

}

void base64_encode_(const unsigned char *input, int length, char *output) {
    const char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    while (length > 2) {
        output[j++] = base64_table[input[i] >> 2];
        output[j++] = base64_table[((input[i] & 0x03) << 4) | (input[i + 1] >> 4)];
        output[j++] = base64_table[((input[i + 1] & 0x0f) << 2) | (input[i + 2] >> 6)];
        output[j++] = base64_table[input[i + 2] & 0x3f];
        length -= 3;
        i += 3;
    }
    if (length != 0) {
        output[j++] = base64_table[input[i] >> 2];
        if (length > 1) {
            output[j++] = base64_table[((input[i] & 0x03) << 4) | (input[i + 1] >> 4)];
            output[j++] = base64_table[(input[i + 1] & 0x0f) << 2];
            output[j++] = '=';
        } else {
            output[j++] = base64_table[(input[i] & 0x03) << 4];
            output[j++] = '=';
            output[j++] = '=';
        }
    }
    output[j] = '\0';
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
    long read_size = 96;
    unsigned long mem_buffer[5024];
    unsigned long long exec_addr = _trace_find_exec_addr(pid->valueint);
    struct iovec *local_v;
    local_v = _trace_proc_mem_read(pid->valueint, exec_addr, read_size);
    if(local_v == NULL) return -1;
    char mem_encoded[read_size * 4 / 3 + 4];

    base64_encode_(local_v->iov_base, read_size, mem_encoded);
    free(local_v->iov_base);
    free(local_v);

    JSON *resp = json_init("empty", NULL);
    if(resp == NULL)
    {
        printf("could not allocate memory for response\n");
        return 1;
    }

    JSON *resp_ = json_init("empty", NULL);
    if(resp_ == NULL)
    {
        printf("could not allocate memory for response\n");
        json_delete(resp);
        return 1;
    }

    JSON *action = json_get_value(args, "actid");
    cJSON_AddNumberToObject(resp, "actid", action->valueint);

    cJSON_AddStringToObject(resp_, "mem", mem_encoded);
    cJSON_AddNumberToObject(resp_, "len", read_size);
    cJSON_AddItemToObject(resp, "resp", resp_);

    *resp_str = cJSON_PrintUnformatted(resp);
    json_delete(resp);
    
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

int proc_func_all(CDB *cdb, JSON *args, char **resp_str)
{
    printf("-------------------------------\n");
    printf("generic: action handler: proc_func_all\n");
    if (cdb  == NULL || args  == NULL) return -1;
    JSON *pid = json_get_value(args, "pid");
    if (pid == NULL)
    {
        printf("process id not provided. no regs\n");
        return -1;
    }
    int proc_exists = cdb_has_proc(cdb, pid->valueint);
    if(!proc_exists) return -1;
    PROCESS *proc = cdb_find_proc(cdb, pid->valueint);
    if(proc == NULL)  
    {
        printf("process object not found\n");
        return -1;
    }
    
    FUNC_INFO *funcs = NULL;
    int funcs_total;
    func_find_all(proc->dw_dbg, &funcs, &funcs_total);
    printf("total functions: %d\n", funcs_total);
    if(funcs == NULL) return -1;

    JSON *resp = json_init("empty", NULL);
    if(resp == NULL) 
    {
        free(funcs);
        printf("(resp) could not allocate memory for response\n");
        return -1;
    }

    JSON *resp_ = json_init("empty", NULL);
    if(resp_ == NULL) 
    {
        free(funcs);
        json_delete(resp);
        printf("(resp_) could not allocate memory for response\n");
        return -1;
    }
    JSON *funcs_json = json_init("empty", NULL);
    if(funcs_json == NULL)
    {
        printf("(funcs) could not allocate memory for response\n");
        free(funcs);
        json_delete(resp);
        return 1;
    }
    for (size_t i = 0; i < funcs_total; i++)
    {
        FUNC_INFO *func_info = (funcs + i);
        func_info_print(func_info);
        JSON *func = json_init("empty", NULL);
        if(func)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "0x%llx", func_info->low_pc);
            cJSON_AddStringToObject(func, "func_name", func_info->func_name);
            cJSON_AddStringToObject(func, "low_pc", buffer);
            cJSON_AddItemToObject(funcs_json, func_info->func_name, func);
        }
    }

    JSON *action = json_get_value(args, "actid");
    cJSON_AddNumberToObject(resp, "actid", action->valueint);

    cJSON_AddItemToObject(resp_, "funcs", funcs_json);
    cJSON_AddItemToObject(resp, "resp", resp_);

    *resp_str = cJSON_PrintUnformatted(resp);
    json_delete(resp);
    return 0;
}

int proc_func_single(CDB *cdb, JSON *args, char **resp_str)
{
    return 0;
}

int no_action()
{
    return 0;
}