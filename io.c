#include <stddef.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <poll.h>
#include "io.h"
#include "util.h"

static struct pollfd epoll_events[MAX_IO_ENTITIES];
static E_IO   *io_entities[MAX_IO_ENTITIES];
static int epoll_fd;
static int io_fds[MAX_IO_ENTITIES];
static int io_curs = 0;

int addnewIOEnt(E_IO *event)
{
    if(event == NULL)
    {
        printf("event is null\n");
        return -1;
    } 
    if(io_curs > (MAX_IO_ENTITIES - 1))
    {
        printf("reached max ios\n");
        return -1;
    }
    event->pos           = io_curs;
    printf("adding e_io to index: %d\n", io_curs);
    io_entities[io_curs] = event;
    io_fds[io_curs]      = event->sys_fd;
    io_curs++;
    printf("curs updated to: %d\n", io_curs);
    return 0;
}

int removeIOEnt(E_IO *event)
{
    return 0;
}

// handle connected fd
void connEventHandler(E_IO *io, struct pollfd *event)
{
    printf("CLient event handler running\n");
    // do some error checking
    E_IO_CONN *io_con = (E_IO_CONN*)io;
    int client_socket = io_con->sys_fd;
    char buffer[BUFFER_SIZE];

    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        return;
    }

    // Decode the WebSocket frame
    unsigned char *frame = (unsigned char *)buffer;
    unsigned char opcode = frame[0] & 0x0F;
    unsigned char masked = frame[1] & 0x80;
    uint64_t payload_length = frame[1] & 0x7F;

    int offset = 2;
    if (payload_length == 126) {
        payload_length = ntohs(*(uint16_t *)(frame + offset));
        offset += 2;
    } else if (payload_length == 127) {
        payload_length = be64toh(*(uint64_t *)(frame + offset));
        offset += 8;
    }

    unsigned char masking_key[4];
    if (masked) {
        memcpy(masking_key, frame + offset, 4);
        offset += 4;
    }

    unsigned char payload[BUFFER_SIZE];
    memcpy(payload, frame + offset, payload_length);

    if (masked) {
        for (uint64_t i = 0; i < payload_length; i++) {
            payload[i] ^= masking_key[i % 4];
        }
    }

    // Handle JSON message
    cJSON *json = cJSON_Parse((char *)payload);
    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return;
    }

    cJSON *command = cJSON_GetObjectItemCaseSensitive(json, "command");
    cJSON *args = cJSON_GetObjectItemCaseSensitive(json, "args");

    if (cJSON_IsString(command) && command->valuestring != NULL) {
        printf("Command: %s\n", command->valuestring);
        cJSON *arg = cJSON_GetObjectItemCaseSensitive(args, "pid");
        if(arg){
            if (cJSON_IsString(arg)) {
                printf("(string) Arg %s: %s\n", arg->string, arg->valuestring);
                // startTracer(atoi(arg->valuestring));
            } else if (cJSON_IsNumber(arg)) {
                printf("(number) Arg %s: %lf\n", arg->string, arg->valuedouble);
            }
        }else{
            printf("arg is null 'startTracer' not called\n" );
        }
    }else{
        printf("Command: command is not string\n" );
    }

    cJSON_Delete(json);
    printf("Deleted Json object");

    // Echo the message back
    unsigned char response_frame[BUFFER_SIZE];
    response_frame[0] = 0x81; // FIN bit set and text frame opcode (0x1)
    if (payload_length <= 125) {
        response_frame[1] = (unsigned char)payload_length;
        offset = 2;
    } else if (payload_length <= 65535) {
        response_frame[1] = 126;
        *(uint16_t *)(response_frame + 2) = htons((uint16_t)payload_length);
        offset = 4;
    } else {
        response_frame[1] = 127;
        *(uint64_t *)(response_frame + 2) = htobe64(payload_length);
        offset = 10;
    }
    fprintf(stdout, "Sending Back Payload\n");
    memcpy(response_frame + offset, payload, payload_length);
    send(client_socket, response_frame, offset + payload_length, 0);

}

void addFdToEpoll(E_IO *io)
{
    // if(!epoll_fd )
    // {
    //     perror("addFdToEpoll: epoll_fd not initialised");
    //     exit(EXIT_FAILURE);
    // }
    if(io == NULL)
    {
        perror("addFdToEpoll: fd not valid");
        exit(EXIT_FAILURE);
    }
    epoll_events[io->pos].fd = io->sys_fd;
    epoll_events[io->pos].events = POLLIN;
}

void init_ws_conn(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';

    const char *websocket_key_header = "Sec-WebSocket-Key: ";
    char *key_start = strstr(buffer, websocket_key_header);
    if (!key_start) {
        fprintf(stderr, "No WebSocket key found\n");
        close(client_socket);
        return;
    }
    key_start += strlen(websocket_key_header);
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) {
        fprintf(stderr, "Invalid WebSocket key\n");
        close(client_socket);
        return;
    }
// #include <openssl/sha.h>
    char websocket_key[128];
    snprintf(websocket_key, key_end - key_start + 1, "%s", key_start);

    const char *magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concatenated[256];
    snprintf(concatenated, sizeof(concatenated), "%s%s", websocket_key, magic_string);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    sha1(concatenated, strlen(concatenated), sha1_result);

    char websocket_accept[256];
    base64_encode(sha1_result, SHA_DIGEST_LENGTH, websocket_accept);

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             websocket_accept);

    // do some error checking
    printf("Client Connected\n");
    send(client_socket, response, strlen(response), 0);
}

void initEntIOConn(int sys_fd)
{
    E_IO_CONN con_io = {
        .sys_fd = sys_fd,
        .eventHandler = &connEventHandler
    };
    init_ws_conn(sys_fd);

    if(addnewIOEnt((E_IO*)&con_io) == -1)
    {
        perror("initEntIOConn: error saving new IOEnt");
        exit(EXIT_FAILURE);
    }
    addFdToEpoll((E_IO *)&con_io);
    printf("Client fd added to epoll. Index: %d\n", con_io.pos);
}


void initEntIOServ()
{
    int sys_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sys_fd == -1) {
        perror("initEntIOServ: socket");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Server Started\n");
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sys_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("initEntIOServ: bind");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(sys_fd, 5) < 0) {
        perror("initEntIOServ: listen");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }
    E_IO_SERV serv_io = {
        .sys_fd = sys_fd,
        .eventHandler = &servEventHandler
    };

    if(addnewIOEnt((E_IO*)&serv_io) == -1)
    {
        perror("initEntIOServ: error saving new IOEnt");
        exit(EXIT_FAILURE);
    }
    addFdToEpoll((E_IO*)&serv_io);
    printf("Server fd added to epoll. index: %d\n", serv_io.pos);
}

void servEventHandler(E_IO *io, struct pollfd *event)
{
    // do some error checking
    struct sockaddr_in clnt_sck;
    socklen_t clnt_sck_size = sizeof(clnt_sck); 
    int con = accept(io->sys_fd, (struct sockaddr *)&clnt_sck, &clnt_sck_size);
    if(con == -1)
    {
        perror("servEventHandler: error connecting with client");
        exit(EXIT_FAILURE);
    }
    initEntIOConn(con);
    printf("server event handler done\n");
}


void initEpoll()
{
    // epoll_fd = epoll_create(MAX_IO_ENTITIES);
    // if(epoll_fd == -1)
    // {
    //     perror("initEpoll: error creating epoll instance");
    //     exit(EXIT_FAILURE);
    // }
}



void mainIO()
{
    initEpoll();
    initEntIOServ();

    int nfds;
    for (;;)
    {
        nfds = poll(epoll_events, MAX_IO_ENTITIES, 1);
        if(nfds > 0)
        {
            for (size_t i = 0; i < MAX_IO_ENTITIES; i++)
            {
                if(epoll_events[i].revents != 0)
                {
                    E_IO *io = io_entities[i];
                    printf("epoll event handler found. Index: %ld\n", i);
                    io->eventHandler(io, &epoll_events[i]);
                }
            }

        }

    }
}


// int main()
// {
//     return 0;
// }