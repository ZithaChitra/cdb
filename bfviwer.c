#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define WORD long

pid_t traceePid;
unsigned long long executableAddr(pid_t pid)
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

int isStillTraced(pid_t tracee) {
    siginfo_t siginfo;

    // Try to get signal information from the tracee process
    if (ptrace(PTRACE_GETSIGINFO, tracee, NULL, &siginfo) == -1) {
        if (errno == ESRCH) {
            return 0;
        } else {
            perror("ptrace(PTRACE_GETSIGINFO)");
            exit(1);
        }
    }
    return 1;
}

void detatch()
{
    if(traceePid && isStillTraced(traceePid))
    {
        if(ptrace(PTRACE_DETACH, traceePid, NULL, NULL) == -1)
        {
            perror("detach: error occurred while detaching from tracee");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "\n[+] detached from tracee\n");
    }else{
        fprintf(stdout, "\n[+] no tracee to detach\n");
    }
    // exit(EXIT_SUCCESS);
}


void tracerRead(pid_t traceePid, unsigned long long startAddr, void *data, size_t len)
{
    if((!traceePid) || (!startAddr) || (!data) || (!len))
    {
        perror("traceeRead: too few args provided");
        exit(EXIT_FAILURE);
    }
     
    long word = 0;
    size_t i  = 0, nw = 0; // next-word
    WORD *ptr = (WORD *)data;


    for (; i < len; i++, nw += sizeof(WORD), word = 0)
    {
        if((word = ptrace(PTRACE_PEEKTEXT, traceePid, startAddr + i, NULL)) == -1)
        {
            perror("[x] traceeRead: error reading from memory");
            exit(EXIT_FAILURE);
        }
        *(ptr + nw) = word;
    }
    return;
}


void traceAttach(pid_t pid)
{
    if(!pid)
    {
        perror("tracerAttach: pid not privided");
        exit(EXIT_FAILURE);
    }

    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1)
    {
        perror("tracerAttach: could not attach to process");
        exit(EXIT_FAILURE);
    }
    int status;
    waitpid(pid, &status, WUNTRACED);
    return;
}


int startTracer(pid_t pid)
{
    struct sigaction sa;
    sa.sa_handler = detatch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    unsigned WORD traceeDataBuffer[5024];

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("main: could not setup sigaction.");
        exit(EXIT_FAILURE);
    }
    
    if(!pid)
    {
        perror("main: too few args provided");
        exit(EXIT_FAILURE);
    }

    traceePid = pid;
    fprintf(stdout, "tracer function has started\n");
    fprintf(stdout, "tracee pid: %d\n", traceePid);
    // return 0;

    unsigned long long freeAddr = executableAddr(traceePid);
    fprintf(stdout, "executable address: %llx\n", freeAddr);

    traceAttach(traceePid);
    tracerRead(traceePid, freeAddr, traceeDataBuffer, 5);
    fprintf(stdout, "\n\n-------------printting tracee data buffer---------------\n");
    long total = sizeof(WORD) * 5;
    for (size_t i = 0; i < total; i += sizeof(WORD))
    {
        fprintf(stdout, "value: %lx\n", *(traceeDataBuffer + i));
        
    }
    
    detatch();
    return 0;
}



















