#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "trace.h"

#ifndef WORD
#define WORD long
#endif

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

unsigned long long _trace_find_exec_addr(pid_t pid)
{
    if(!pid)
    {
        perror("executableAddr: pid not privided");
        exit(EXIT_FAILURE);
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
        exit(1);
    }

    while(fgets(line, sizeof(line), f) != NULL)
    {
        sscanf(line, "%lx-%*lx %s %*s %s %*d", &address, perms, str);
        printf("%s", line);
        if(strstr(line, "x") != NULL){
            break;
        }
    } 
    fclose(f);
    return address;
}

void _trace_proc_mem_read(pid_t pid, unsigned long long start_addr, void *data, size_t len)
{
    if((!pid) || (!start_addr) || (!data) || (!len))
    {
        perror("traceeRead: too few args provided");
        exit(EXIT_FAILURE);
    }
     
    long word = 0;
    size_t i  = 0, nw = 0; // next-word
    WORD *ptr = (WORD *)data;

    for (; i < len; i++, word = 0)
    {
        if((word = ptrace(PTRACE_PEEKTEXT, pid, start_addr + i, NULL)) == -1)
        {
            perror("[x] traceeRead: error reading from memory");
            exit(EXIT_FAILURE);
        }
        *(ptr + (i * sizeof(WORD))) = word;
    }
    return;
}

