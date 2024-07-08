#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>
#include <string.h>
#include <stdlib.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <sys/uio.h>
#include <zlib.h>
#include "generic.h"
#include "cdb.h"
#include "json.h"
#include "trace.h"
#include "resolve.h"
#include "data/hashmap.h"

// action handler def
#define ACT_HANDLR_START(handler_name)\
    int handler_name(CDB *cdb, JSON *args, char **resp_str) \
    {   \
        if(cdb == NULL || args == NULL) return -1;  \
        JSON *action = json_get_value(args, "actid"); \
        JSON *pid = json_get_value(args, "pid");    \
        if (pid == NULL)    \
        {   \
            fprintf(stderr, "process id not provided.\n");  \
            return -1;  \
        }   \
        int proc_exists = cdb_has_proc(cdb, pid->valueint); \
        JSON *resp = json_init("empty", NULL);  \
        if(resp == NULL)    \
        {   \
            fprintf(stderr, "could not allocate memory for response json.\n"); \
            return -1; \
        }   \
        cJSON_AddNumberToObject(resp, "actid", action->valueint);   \
        JSON *resp_ = json_init("empty", NULL); \
        if(resp_ == NULL)   \
        {   \
            printf("could not allocate memory for response data\n"); \
            json_delete(resp);  \
            return -1;  \
        }   \
        cJSON_AddItemToObject(resp, "resp", resp_); \


#define ACT_HANDLR_END \
        *resp_str = cJSON_PrintUnformatted(resp); \
        printf("%s\n", *resp_str);  \
        printf("-------------------------------\n");  \
        json_delete(resp);  \
        return 0;   \
        failure:    \
            json_delete(resp);  \
            return -1;   \
    }   \


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

ACT_HANDLR_START(proc_end_dbg)
    if(proc_exists == 1)
    {
        printf("process already attached\n");
        if(_trace_proc_cont_kill(pid->valueint) == -1) return -1;
        if(cdb_remove_proc(cdb, pid->valueint) == -1) return -1;
        return -1;
    }
    // TODO
ACT_HANDLR_END

ACT_HANDLR_START(proc_regs_read)
    if(!proc_exists) return -1;
    struct user_regs_struct *regs = _trace_proc_get_regs(pid->valueint);
    if(regs == NULL) goto failure;
    JSON *regs_json = json_init("empty", NULL);
    if(regs_json == NULL)
    {
        printf("could not allocate memory for response\n");
        free(regs);
        goto failure;
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

    cJSON_AddNumberToObject(resp_, "pid", pid->valueint);
    cJSON_AddItemToObject((cJSON *)resp_, "regs", regs_json);

    free(regs);
ACT_HANDLR_END

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


ACT_HANDLR_START(proc_mem_read)
    if(!proc_exists)
    {
        json_delete(resp);
        return -1;
    };
    long read_size = 10000;
    char mem_encoded[read_size * 4 / 3 + 4];


    PROCESS *proc = cdb_find_proc(cdb, pid->valueint);
    if(proc == NULL) goto failure;

    JSON *addr = json_get_value(args, "read_address"); 
    void *read_addr = 0;
    if(addr == NULL)
    {
        read_addr = proc->base_addr;
        printf("proc exec: %p\n", proc->base_addr);
        printf("proc read_address not set\n");
    }else{
        read_addr = (char *)proc->base_addr + strtoull(addr->valuestring, NULL, 16);
        printf("proc exec: %p\n", proc->base_addr);
        printf("proc read_address: %p\n", read_addr);
    }

    struct iovec *local_v;
    local_v = _trace_proc_mem_read(pid->valueint, read_addr, read_size);
    if(local_v == NULL)
    {
        goto failure;
    }

    size_t compressed_size = compressBound(local_v->iov_len + 1);
    char *compressed_data = (char *)malloc(compressed_size);
    int res = compress((Bytef*)compressed_data, &compressed_size, 
                (Bytef*)local_v->iov_base, local_v->iov_len);
    if(res != Z_OK)
    {
           
    }   

    base64_encode_(compressed_data, compressed_size, mem_encoded);

    cJSON_AddStringToObject(resp_, "mem", mem_encoded);
    cJSON_AddNumberToObject(resp_, "len", read_size);

    free(local_v->iov_base);
    free(local_v);
    free(compressed_data);
ACT_HANDLR_END


ACT_HANDLR_START(proc_mem_write)
    printf("-------------------------------\n");
    printf("generic: proc_mem_write\n");
    if(!proc_exists) goto failure;
    unsigned long long base_addr = _trace_find_base_addr(pid->valueint);
    printf("(write mem) exec address- %llu\n", base_addr);
    unsigned char trap = 0xCC;
    int res = _trace_proc_mem_write(pid->valueint, (void *)base_addr, &trap, sizeof(trap));
    if(res < 0)
    {
        int err_code = -res;
        printf("could not write to proc memory: %s\n", strerror(err_code));
        goto failure;
    } 
    printf("wrote to proc memory\n");
    json_delete(resp);
    return proc_mem_read(cdb, args, resp_str);
ACT_HANDLR_END

int proc_step_single(CDB *cdb, JSON *args, char **resp_str)
{
    printf("handler: proc_step_single");
    JSON *pid = json_get_value(args, "pid");
    if(pid == NULL) return -1;
    if (_trace_proc_step_single(pid->valueint) == -1) return -1;
    return proc_regs_read(cdb, args, resp_str);
}

ACT_HANDLR_START(proc_func_all)
    printf("-------------------------------\n");
    printf("generic: action handler: proc_func_all\n");
    if(!proc_exists) goto failure;
    PROCESS *proc = cdb_find_proc(cdb, pid->valueint);
    if(proc == NULL)  
    {
        printf("process object not found\n");
        goto failure;
    }
    
    int funcs_total;
    FUNC_INFO **funcs = func_find_all(proc->dw_dbg, &funcs_total);
    
    printf("total functions: %d\n", funcs_total);
    if(funcs == NULL) goto failure;

    JSON *funcs_json = json_init("empty", NULL);
    if(funcs_json == NULL)
    {
        printf("(funcs) could not allocate memory for response\n");
        free(funcs);
        goto failure;
    }
    for (size_t i = 0; i < funcs_total; i++)
    {
        FUNC_INFO *func_info = *(funcs + i);
        if(func_info == NULL) continue;
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
    cJSON_AddItemToObject(resp_, "funcs", funcs_json);
ACT_HANDLR_END

int proc_func_single(CDB *cdb, JSON *args, char **resp_str)
{
    return 0;
}

ACT_HANDLR_START(proc_break)
    if(!proc_exists) goto failure;
    PROCESS *proc = cdb_find_proc(cdb, pid->valueint);
    if(proc == NULL) goto failure;

    JSON *break_addr = json_get_value(args, "break_address");
    if(break_addr == NULL) goto failure;
    void *addr = (char *)proc->base_addr + strtoull(break_addr->valuestring, NULL, 16);
    printf("----------------\n");
    printf("proc->base_address: %p\n", proc->base_addr);
    printf("setting break address: %p\n", addr);
    printf("----------------\n");
    long og_code = _trace_proc_break(pid->valueint, addr);
    if(og_code == -1)
    {
        goto failure;
    }

    int index = proc_add_breakp(proc, &addr, og_code);
    cJSON_AddStringToObject(args, "read_address", break_addr->valuestring);
    
    printf("bp added at index: %d\n", index);
    json_delete(resp);
    return proc_mem_read(cdb, args, resp_str);
ACT_HANDLR_END



ACT_HANDLR_START(proc_cont)
    if(!proc_exists) goto failure;
    PROCESS *proc = cdb_find_proc(cdb, pid->valueint);
    if(proc == NULL) goto failure;

    if(_trace_proc_cont(pid->valueint) == -1)
    {
        goto failure;
    }
    struct user_regs_struct *regs = _trace_proc_get_regs(pid->valueint);
    if(regs == NULL) goto failure;

    regs->rip -= 1;
    unsigned long long rip = regs->rip;
    if (_trace_proc_set_regs(pid->valueint, regs) == -1)
    {
        free(regs);
        goto failure;
    }
    
    char addr[100];
    char ogcode_str[100];
    snprintf(addr, sizeof(addr), "%llx", rip);

    BREAKP *bp = proc_find_breakp(proc, (void *)rip);
    snprintf(ogcode_str, 100, "%lx", bp == NULL ? 0 : bp->og_code);


    if(bp)
    {
        if(_trace_proc_rm_break(pid->valueint, bp->addr, bp->og_code) == 1)
        {
            printf("could not write to trace mem\n");
        }
    }

    cJSON_AddStringToObject(resp_, "bp_ip", addr);
    cJSON_AddStringToObject(resp_, "og_code", ogcode_str);
    free(regs);
ACT_HANDLR_END

int no_action()
{
    return 0;
}