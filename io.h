#include <poll.h>

#ifndef IO_H
#define IO_H

#define MAX_IO_ENTITIES 2

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef PORT
#define PORT 8080
#endif

typedef struct E_IO
{
    int sys_fd;
    int pos;
    void (*eventHandler)(void *self, struct pollfd *);
} E_IO;

typedef struct E_IO_SERV    
{
    int sys_fd;
    int pos;
    void (*eventHandler)(E_IO *, struct pollfd *);
} E_IO_SERV;

typedef struct E_IO_CONN
{
    int sys_fd;
    int pos;
    void (*eventHandler)(E_IO *, struct pollfd *);
} E_IO_CONN;


int addnewIOEnt(E_IO *);
int removeIOEnt(E_IO *);

void connEventHandler(E_IO*, struct pollfd*);
void servEventHandler(E_IO*, struct pollfd*);

void mainIO();

#endif
