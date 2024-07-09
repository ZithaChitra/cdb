#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>
#include "trace.h"

#ifndef WORD
#define WORD long
#endif

extern char **environ;

int _trace_proc_start(char *procpath)
{
    printf("running: _trace_proc_start: %s\n", procpath);
    pid_t pid;
    int status;

    pid = fork();     
    if(pid == -1)
    {
        printf("_trace_start_proc: could not fock\n ");
        return -1;
    }else if(pid == 0)
    {
        printf("child: requesting trace me\n");
        if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
        {
            printf("_trace_start_proc: could not set traceme \n");
            return -1;
        }
        char *testpath = "/media/pnorth/Chitra/cdb/cdb-server/build/demo";
        char *args[2];
        args[0] = "demo";
        args[1] = NULL;
        char fullp[100];
        getcwd(fullp, 100);
        if(execv("demo", environ) == -1)
        {
            int err = errno;
            printf("execvp failed: %s, err: %d\n\ncwd: %s\n", testpath, err, fullp);
        }
        return -1;
    }else{
        printf("parent: waiting for child\n");
        waitpid(pid, &status, 0);

        return pid;
    }
    return -1;
}

void print_stack_frames(pid_t child_pid) 
{
    unw_addr_space_t as;
    unw_cursor_t cursor;
    struct UPT_info *context;

    as = unw_create_addr_space(&_UPT_accessors, 0);
    if (!as) 
    {
        fprintf(stderr, "unw_create_addr_space failed\n");
        return;
    }

    context = _UPT_create(child_pid);
    if (!context) 
    {
        fprintf(stderr, "_UPT_create failed\n");
        unw_destroy_addr_space(as);
        return;
    }

    if (unw_init_remote(&cursor, as, context) != 0) 
    {
        fprintf(stderr, "unw_init_remote failed\n");
        _UPT_destroy(context);
        unw_destroy_addr_space(as);
        return;
    }

    unw_word_t ip, sp;
    char proc_name[256];
    unw_word_t offset;
    int frame = 0;
    do {
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) 
            || unw_get_reg(&cursor, UNW_REG_SP, &sp)) 
        {
            fprintf(stderr, "unw_get_reg failed\n");
            break;
        }

        if (unw_get_proc_name(&cursor, proc_name, sizeof(proc_name), &offset) == 0) 
        {
            printf("Frame %d: ip = 0x%lx, sp = 0x%lx, function = %s+0x%lx\n", 
                   frame, (long)ip, (long)sp, proc_name, (long)offset);
        } else {
            printf("Frame %d: ip = 0x%lx, sp = 0x%lx, function = <unknown>\n", 
                   frame, (long)ip, (long)sp);
        }

        frame++;
    } while (unw_step(&cursor) > 0);

    _UPT_destroy(context);
    unw_destroy_addr_space(as);
}

int _trace_proc_cont(pid_t pid)
{
    if(ptrace(PTRACE_CONT, pid, NULL, NULL) == -1)
    {
        printf("(ptrace) could not continue\n");
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    if(WIFEXITED(status)) 
    {
        printf("(exited) could not continue\n");
        return -1;
    }
    print_stack_frames(pid);

    
    return 0;
}

void _trace_proc_kill(pid_t pid)
{
    ptrace(PTRACE_KILL, pid, NULL, NULL);
    return;
}


unsigned long long _trace_proc_get_ip(pid_t pid)
{
    struct user_regs_struct regs;
    if(ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
    {
        printf("could not get ip\n");
        return -1;
    }
    return regs.rip;
}

// continue &/or kill
int _trace_proc_cont_kill(pid_t pid)
{
    if(_trace_is_proc_attached(pid))
    {
        if(ptrace(PTRACE_CONT, pid, NULL, NULL) == -1)
        {
            perror(" _trace_proc_cont_kill: error running proc\n");
            return -1;
        }
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status))
        {
            return 0;
        }

        if(kill(pid, SIGKILL) == -1)
        {
            perror(" _trace_proc_cont_kill: error terminating proc\n");
            return -1;
        }
        return 0;
    }
    return -1;
}


int _trace_attach(pid_t pid)
{
    int attached = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if(attached == -1) return -1;
    
    int status;
    waitpid(pid, &status, WUNTRACED );
    printf("[sucess] attched to pid: %d\n", pid);
    return 0;
}

int _trace_detach(pid_t pid)
{
    if(_trace_is_proc_attached(pid))
    {
        if(ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1)
        {
            printf("[fail] could not detach from pid: %d\n", pid);
            return 0;
        };
        printf("[success] detached from pid: %d\n", pid);
        return 1;
    }
    return 0;
}

int _trace_is_proc_attached(pid_t pid)
{
    siginfo_t siginfo;

    // Try to get signal information from the tracee process
    if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo) == -1) {
        if (errno == ESRCH) {
            return 0;
        } // TODO: handler more errors 
    }
    return 1;
}

int _trace_proc_step_single(pid_t pid)
{
    if(ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == 1)
    {
        printf("error single stepping tracee\n");
        return -1;
    }
    int status;
    waitpid(pid, &status, 0);
    return 0;
}

struct user_regs_struct *_trace_proc_get_regs(pid_t pid)
{
    struct user_regs_struct *regs = 
        (struct user_regs_struct *)malloc(sizeof(struct user_regs_struct));
    if(regs == NULL) return NULL;
    if(ptrace(PTRACE_GETREGS, pid, NULL, regs) == -1)
    {
        free(regs);
        return NULL;
    }
    return regs;
}

int _trace_proc_set_regs(pid_t pid, struct user_regs_struct *regs)
{
    if(pid && regs)
    {
        return ptrace(PTRACE_SETREGS, pid, NULL, regs);
    }
    return -1;
}


void *_trace_find_base_addr(pid_t pid)
{
    if(!pid)
    {
        perror("executableAddr: pid not privided");
        return NULL;
    }

    FILE *f;
    char fileName[100];
    char line[1024];
    char perms[50];
    unsigned long address = 0;
    char str[20];

    sprintf(fileName, "/proc/%d/maps", pid);

    if((f = fopen(fileName, "r")) == NULL)
    {
        perror("executableAddr: could not open proc 'map' file");
        return NULL;
    }

    while(fgets(line, sizeof(line), f) != NULL)
    {
        sscanf(line, "%lx-%*lx %s %*s %s %*d", &address, perms, str);
        printf("%s", line);
        if(strstr(line, "r") != NULL){
            break;
        }
    } 
    fclose(f);
    return (void *)address;
}

struct iovec *_trace_proc_mem_read(pid_t pid, void *remote_addr, size_t len)
{
    if(pid && remote_addr && len) 
    {
        char *local_buff = (char *)malloc(len);
        if(local_buff == NULL) return NULL;
        struct iovec *local_v = (struct iovec *)malloc(sizeof(struct iovec));
        if(local_v == NULL)
        {
            free(local_buff);
            return NULL;
        }
        local_v->iov_base = local_buff;
        local_v->iov_len = len;

        struct iovec remote_v = {
            .iov_base = remote_addr,
            .iov_len  = len
        };
        ssize_t nread = process_vm_readv(pid, local_v, 1, &remote_v, 1, 0);
        if(nread == -1)
        {
            printf("_trace_proc_mem_read: error rading tracee mem\n");
            free(local_v);
            free(local_buff);
            return NULL;
        }
        return local_v;
    }
    return NULL;
}


int _trace_proc_mem_write(pid_t pid, void *remote_addr, void *local_addr, size_t len)
{
    errno = 0;
    struct iovec local_iovec = {
        .iov_base   = local_addr,
        .iov_len    = len
    };
    struct iovec remote_iovec = {
        .iov_base   = remote_addr,
        .iov_len    = len
    };

    size_t b_written = process_vm_writev(pid, &local_iovec, 1, &remote_iovec, 1, 0);
    if(b_written == -1) 
    {
        printf("could not write to child process memory\n");
    }
    return b_written;
}



long _trace_proc_break(pid_t pid, void *remote_addr)
{
    errno = 0;
    if(pid && remote_addr)
    {
        long og_code = ptrace(PTRACE_PEEKTEXT, pid, remote_addr, NULL);
        if(og_code == -1) return -1;

        long md_code = (og_code & 0xFFFFFFFFFFFFFF00) | 0xCC;
        int res      = ptrace(PTRACE_POKETEXT, pid, remote_addr, md_code);
        if(res == -1) return -1;
        return og_code;
    }
    return -1;
}


int _trace_proc_rm_break(pid_t pid, void *remote_addr, long og_code)
{
    if(remote_addr && og_code)
    {
        return ptrace(PTRACE_POKETEXT, pid, remote_addr, og_code);
    }
    return -1;
}



















