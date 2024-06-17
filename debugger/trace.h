#ifndef _CDB_TRACE
#define _CDB_TRACE

#include <sys/types.h>
#include <sys/user.h>
#include <sys/uio.h>

/**
 * These will be wrappers to platform specific
 * APIs
*/

int _trace_proc_start(char *procpath);
int _trace_proc_cont_kill(pid_t pid);
int _trace_proc_step_single(pid_t pid);
int _trace_attach(pid_t pid);
int _trace_detach(pid_t pid);

int _trace_is_proc_attached(pid_t pid);
struct user_regs_struct *_trace_proc_get_regs(pid_t pid);

unsigned long long _trace_find_exec_addr(pid_t pid);
struct iovec *_trace_proc_mem_read(pid_t pid, unsigned long long addr, size_t len);


#endif
