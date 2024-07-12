#ifndef CDB_IO_H
#define CDB_IO_H

#include <poll.h>
#include "cdb.h"

#define MAX_IO_ENTITIES 5

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 15000
#endif

#ifndef PORT
#define PORT 8080
#endif

struct FD;

typedef void (*HANDLER_FD)(struct FD *self, struct pollfd *);

typedef struct FD
{ 
    int         sys_fd;
    int         pos;
    PROCESS     *proc;               // currently used by STDIO fds for tracee
    HANDLER_FD  event_handler;
} FD;


int fd_add_to_tables(FD *fd);
int fd_rm_from_tables(FD *fd);

FD *fd_init(int sys_fd, HANDLER_FD handler);
int fd_del(FD *fd);

void handle_connected_fd(FD *self, struct pollfd*);
void handle_listening_fd(FD *self, struct pollfd*);
void handle_tracee_stdout(FD *self, struct pollfd*);

int tracee_stdout_fd_init(int sys_fd, PROCESS *proc);

void fds_setup();
void fds_poll();
#endif