#ifndef CDB_SIGNAL_HANDLER
#define CDB_SIGNAL_HANDLER

#include "signal.h"
#include "cdb.h"

// handled signals
typedef enum sigid
{
    SIG_SIGINT,
    SIG_UNKNOWN,

}SIGID;

// struct sigaction *handler_sigint();
void init_signal_handlers(CDB *cdb);


#endif