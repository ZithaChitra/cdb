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
int _trace_proc_set_regs(pid_t pid, struct user_regs_struct *regs);

void *_trace_find_base_addr(pid_t pid);
struct iovec *_trace_proc_mem_read(pid_t pid, void *remote_addr, size_t len);
int _trace_proc_mem_write(pid_t pid, void *remote_addr, void *local_addr, size_t len);
long _trace_proc_break(pid_t pid, void *remote_addr);
int _trace_proc_rm_break(pid_t pid, void *remote_addr, long og_code);
int _trace_proc_cont(pid_t pid);
void  _trace_proc_kill(pid_t pid);
unsigned long long _trace_proc_get_ip(pid_t pid);

#endif
