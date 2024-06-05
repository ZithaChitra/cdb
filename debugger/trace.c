#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include "trace.h"

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