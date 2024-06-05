#ifndef _CDB_TRACE
#define _CDB_TRACE

#include <sys/types.h>
#include <sys/user.h>

/**
 * These will be wrappers to platform specific
 * APIs
*/

int _trace_attach(pid_t pid);
int _trace_detach(pid_t pid);

int _trace_is_proc_attached(pid_t pid);
struct user_regs_struct *_trace_proc_get_regs(pid_t pid);



#endif
