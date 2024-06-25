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

struct E_IO;

typedef void (*Handler_IO)(struct E_IO *self, struct pollfd *);

typedef struct E_IO
{
    int         sys_fd;
    int         pos;
    char        *type;
    Handler_IO  eventHandler;
} E_IO;

typedef struct E_IO_SERV    
{
    E_IO io;
} E_IO_SERV;

typedef struct E_IO_CONN
{
    E_IO io;
} E_IO_CONN;

int addnewIOEnt(E_IO *);
int removeIOEnt(E_IO *);

void initEntIO(E_IO *, Handler_IO, char*);

void connEventHandler(E_IO *self, struct pollfd*);
void servEventHandler(E_IO *self, struct pollfd*);

void mainIO();
#endif