#ifndef IO_H
#define IO_H

#include <poll.h>

#define MAX_IO_ENTITIES 2

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
    HANDLER_FD  event_handler;
} FD;


int add_fd_to_tables(FD *fd);
int rm_fd_from_tables(FD *fd);

FD *fd_init(int sys_fd, HANDLER_FD handler);
int fd_del(FD *fd);

void handle_connected_fd(FD *self, struct pollfd*);
void handle_listening_fd(FD *self, struct pollfd*);

void main_io();
#endif