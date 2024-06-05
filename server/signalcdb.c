#include "signalcdb.h"
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/user.h>
#include "cdb.h"

static struct sigaction *sig_handlers[SIG_UNKNOWN];
static int sig_curs = 0;

struct sigaction *sigaction_init()
{
    struct sigaction *sa = (struct sigaction*)malloc(sizeof(struct sigaction));
    if(sa == NULL) return NULL;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    return sa;
}

void sigaction_save(struct sigaction *sa)
{
    if(sa == NULL) return;
    if(sig_curs < SIG_UNKNOWN)
    {
        sig_handlers[sig_curs] = sa;
        sig_curs++;
    }
    return;
}

struct sigaction *handler_sigint(CDB *cdb)
{
    struct sigaction *sa = sigaction_init();
    sa->sa_handler = cdb_delete;

    if(sigaction(SIGINT, sa, NULL) == -1)
    {
        perror("main: could not setup sigaction.");
        exit(EXIT_FAILURE);
    }
    return sa;
}


void init_signal_handlers(CDB *cdb)
{
    sigaction_save(handler_sigint(cdb));
}