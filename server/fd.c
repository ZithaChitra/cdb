#include <stddef.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <poll.h>
#include "fd.h"
#include "util.h"
#include "actions.h"
#include "json.h"
#include "cdb.h"
#include "signalcdb.h"

extern HANDLER_CDB action_handlers[];

static struct pollfd epoll_events[MAX_IO_ENTITIES];
static FD  *fd_objs_arr[MAX_IO_ENTITIES];

static int fds_free_list[MAX_IO_ENTITIES];
static int fds_inuse_count    = 0;
static int fds_free_list_head = 0;
CDB *cdb_main;


int fd_tables_init()
{
    // dont calloc me.
    for (size_t i = 0; i < sizeof(fd_objs_arr); i++)
    {
        fd_objs_arr[i]  = NULL;  
    }
    return 0;
}


// idx = index
int idx_arr_init(int idx_arr[], size_t max)
{
    if(idx_arr == NULL) return -1;
    for (size_t i = 0; i < max - 1; i++)
        idx_arr[i] = i + 1;

    idx_arr[max - 1] = -1;
    return 0;
}

// find free spot
int idx_arr_find_free(int idx_arr[], int *curr_head) 
{
    if(idx_arr == NULL || curr_head == NULL) return -1;
    if(*curr_head == -1) return -1;

    int idx    = *curr_head;
    *curr_head = idx_arr[*curr_head];
    return idx;
}

// free an index
int idx_arr_free(int idx_arr[], int *curr_head, int idx)
{
    if(idx_arr == NULL || curr_head == NULL) return -1;
    idx_arr[idx] = *curr_head;
    *curr_head   = idx;
    return 0;
}

FD *fd_init(int sys_fd, HANDLER_FD handler)
{
    if(handler == NULL) return NULL;

    FD *fd = (FD *)malloc(sizeof(FD));
    if(fd == NULL) return NULL;

    fd->pos             = -1;
    fd->sys_fd          = sys_fd;
    fd->proc            = NULL;
    fd->event_handler   = handler;
    return fd;
}

int fd_add_to_tables(FD *fd)
{
    if(fd == NULL) return -1;

    int free_idx = idx_arr_find_free(fds_free_list, &fds_free_list_head);
    if(free_idx == -1) return -1;

    fd->pos               = free_idx;
    fd_objs_arr[free_idx] = fd;

    epoll_events[free_idx].fd      = fd->sys_fd;
    epoll_events[free_idx].events  = POLLIN;
    epoll_events[free_idx].revents = 0;
    fds_inuse_count += 1;
    return 0;
}

int fd_rm_from_tables(FD *fd)
{
    if(fd == NULL) return -1;
    int idx = fd->pos;

    epoll_events[idx].fd      = -1;
    epoll_events[idx].events  = 0;
    epoll_events[idx].revents = 0;

    close(fd->sys_fd);  

    fd_objs_arr[idx] = NULL;

    idx_arr_free(fds_free_list, &fds_free_list_head, idx);
    fds_inuse_count -= 1;

    return 0;
}

int fd_del(FD *fd)
{
    if(fd == NULL) return -1;
    close(fd->sys_fd);
    free(fd);
    
    return 0;
}

void connected_fd_cleanup(FD *fd)
{
    fd_rm_from_tables(fd);
    cdb_rm_conn_procs(cdb_main, fd->sys_fd);
    fd_del(fd);
    return;
}

void tracee_stdout_fd_cleanup(FD *fd)
{
    fd_rm_from_tables(fd);
    fd_del(fd);
    return;
}

void ws_send(int fd, void *data, size_t len)
{
    int offset = 2;
    unsigned char response_frame[BUFFER_SIZE];
    response_frame[0] = 0x81; // FIN bit set and text frame opcode (0x1)
    if (len <= 125) 
    {
        response_frame[1] = (unsigned char)len;
        offset = 2;
    } else if (len <= 65535) {
        response_frame[1] = 126;
        *(uint16_t *)(response_frame + 2) = htons((uint16_t)len);
        offset = 4;
    } else {
        response_frame[1] = 127;
        *(uint64_t *)(response_frame + 2) = htobe64(len);
        offset = 10;
    }
    memcpy(response_frame + offset, data, len);
    send(fd, response_frame, offset + len, 0);
    return;
}

void handle_tracee_stdout(FD *fd, struct pollfd *event)
{
    printf("--------STDIO DATA START -----------------------\n");  
    PROCESS *proc = fd->proc; 
    if(proc == NULL)
    {
        // delete this fd 
        return;
    }

    int buffer_size = 2024;
    char buffer[buffer_size];

    if(event->revents & (POLLERR | POLLHUP))
    {
        // TODO: send alert through ws_fd
        printf("READ: STDOUT POLLERR - deleting\n");
        tracee_stdout_fd_cleanup(fd);
        return;
    }

    int bytes_read = read(fd->sys_fd, buffer, buffer_size - 1);
    if(bytes_read == -1)
    {
        // TODO: send alert through ws_fd
        printf("READ: STDOUT ERROR - deleting\n");
        tracee_stdout_fd_cleanup(fd);
        return;
    }

    buffer[bytes_read + 1] = '\0';
    JSON *resp = json_init("empty", NULL);  
    if(resp == NULL)    
    {   
        fprintf(stderr, "could not allocate memory for response json.\n"); 
        return; 
    }  
    cJSON_AddNumberToObject(resp, "actid", PROC_STDOUT);
    JSON *resp_ = json_init("empty", NULL); 
    if(resp_ == NULL)   
    {   
        printf("could not allocate memory for response data\n"); 
        json_delete(resp);  
        return;  
    }   
    cJSON_AddItemToObject(resp, "resp", resp_); 
    cJSON_AddStringToObject(resp_, "output", buffer);
    char *resp_str = cJSON_PrintUnformatted(resp); 
    printf("%s\n", resp_str);  
    printf("--------STDIO DATA END -----------------------\n");  
    ws_send(proc->ws_fd, resp_str, strlen(resp_str));
    json_delete(resp);  
}



void handle_connected_fd(FD *fd, struct pollfd *event)
{
    if(event->revents & (POLLERR | POLLHUP))
    {
        connected_fd_cleanup(fd);
        return;
    }
    if(event->revents & POLLIN)
    {
        int client_socket = fd->sys_fd;
        char buffer[BUFFER_SIZE];

        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            connected_fd_cleanup(fd);
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

        // JSON *json = cdb_exec_action(cdb, (char*)payload);
        JSON *json = json_from_string((char*)payload);
        char *resp_str = NULL;
        json_print(json);
        // // // cJSON *json = cJSON_Parse((char *)payload);
        if (json == NULL) {
            fprintf(stderr, "Error parsing JSON\n");
            return;
        }else{
            cdb_exec_action(cdb_main, json, &resp_str, fd->sys_fd);
        }

        // cJSON_Delete(json);
        if(json != NULL) json_delete(json);
        // json_delete(json);
        printf("Deleted Json object");
        if(resp_str == NULL) resp_str = "something went wrong";
        uint64_t resp_length = strlen(resp_str);

        ws_send(fd->sys_fd, resp_str, resp_length);
    } 
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

    // TODO: do some error checking
    printf("Client Connected\n");
    send(client_socket, response, strlen(response), 0);
}

int connected_fd_init(int sys_fd)
{
    FD *fd = fd_init(sys_fd, &handle_connected_fd);

    if(fd == NULL) return -1;
    if(fd_add_to_tables(fd) == -1) return -1;

    init_ws_conn(sys_fd);
    return 0;
}

int tracee_stdout_fd_init(int sys_fd, PROCESS *proc)
{
    FD *fd = fd_init(sys_fd, &handle_tracee_stdout);

    if(fd == NULL) return -1;
    fd->proc = proc;

    if(fd_add_to_tables(fd) == -1) return -1;

    init_ws_conn(sys_fd);
    return 0;
}

void handle_listening_fd(FD *fd, struct pollfd *event)
{
    if(event->revents & (POLLHUP | POLLERR))
    {
        perror("handle listening fd");
        exit(EXIT_FAILURE);
        // fd_rm_from_tables(fd);
        // fd_del(fd);
        return;
    }

    if(event->revents & POLLIN)
    {
        struct sockaddr_in clnt_sck;
        socklen_t clnt_sck_size = sizeof(clnt_sck); 
        int con = accept(fd->sys_fd, (struct sockaddr *)&clnt_sck, &clnt_sck_size);
        if(con == -1)
        {
            perror("handle_listening_fd: error connecting with client");
            return;
        }
        connected_fd_init(con);
        printf("server event handler done\n");
    }
}


void sock_main_init()
{
    int sys_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sys_fd == -1) {
        perror("sock_main_init: socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sys_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("sock_main_init: bind");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(sys_fd, MAX_IO_ENTITIES) < 0) 
    {
        perror("sock_main_init: listen");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }

    FD *fd_obj = fd_init(sys_fd, &handle_listening_fd);
    if(fd_obj == NULL)
    {
        perror("sock_main_init: fd_init");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }

    if(fd_add_to_tables(fd_obj) == -1)
    {
        perror("sock_main_init: fd_add_to_tables");
        close(sys_fd);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server Started\n");
    return;
}


void fds_setup()
{
    fd_tables_init();
    idx_arr_init(fds_free_list, MAX_IO_ENTITIES);
}

void fds_poll()
{
    fds_setup();
    sock_main_init();
    cdb_main = cdb_init();

    if (cdb_main == NULL)
    {
        printf("cdb: could not initialize bebugger\n");
    }else{
        printf("cdb: initialized bebugger\n");
    }
    init_signal_handlers(cdb_main);

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
                    FD *fd = fd_objs_arr[i];
                    if(fd == NULL) 
                    {
                        fprintf(stdout, "Handler Not Set");
                        continue;
                    }
                    printf("epoll event handler found. Index: %ld\n", i);
                    fd->event_handler(fd, &epoll_events[i]);
                }
            }
        }
    }
}

