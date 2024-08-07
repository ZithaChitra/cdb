#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <sys/epoll.h>
#include <stdint.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include "fd.h"

#define MAX_EVENTS 10

#define PORT 8080
#define BUFFER_SIZE 15000

int sys_start() {
    printf("updated server has started\n");
    fds_poll();
    return 0;
}
