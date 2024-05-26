#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include "bfviwer.h"

#define PORT 8080
#define BUFFER_SIZE 1024

void base64_encode(const unsigned char *input, int length, char *output) {
    const char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    while (length > 2) {
        output[j++] = base64_table[input[i] >> 2];
        output[j++] = base64_table[((input[i] & 0x03) << 4) | (input[i + 1] >> 4)];
        output[j++] = base64_table[((input[i + 1] & 0x0f) << 2) | (input[i + 2] >> 6)];
        output[j++] = base64_table[input[i + 2] & 0x3f];
        length -= 3;
        i += 3;
    }
    if (length != 0) {
        output[j++] = base64_table[input[i] >> 2];
        if (length > 1) {
            output[j++] = base64_table[((input[i] & 0x03) << 4) | (input[i + 1] >> 4)];
            output[j++] = base64_table[(input[i + 1] & 0x0f) << 2];
            output[j++] = '=';
        } else {
            output[j++] = base64_table[(input[i] & 0x03) << 4];
            output[j++] = '=';
            output[j++] = '=';
        }
    }
    output[j] = '\0';
}

void sha1(const char *input, size_t length, unsigned char *output) {
    SHA_CTX context;
    if (!SHA1_Init(&context)) {
        perror("SHA1_Init failed");
        exit(1);
    }
    if (!SHA1_Update(&context, (unsigned char *)input, length)) {
        perror("SHA1_Update failed");
        exit(1);
    }
    if (!SHA1_Final(output, &context)) {
        perror("SHA1_Final failed");
        exit(1);
    }
}

void handle_client(int client_socket) {
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

    send(client_socket, response, strlen(response), 0);

    // Read and handle WebSocket frames
    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
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
            continue;
        }

        cJSON *command = cJSON_GetObjectItemCaseSensitive(json, "command");
        cJSON *args = cJSON_GetObjectItemCaseSensitive(json, "args");

        if (cJSON_IsString(command) && command->valuestring != NULL) {
            printf("Command: %s\n", command->valuestring);
            cJSON *arg = cJSON_GetObjectItemCaseSensitive(args, "pid");
            if(arg){
                if (cJSON_IsString(arg)) {
                    printf("(string) Arg %s: %s\n", arg->string, arg->valuestring);
                    startTracer(atoi(arg->valuestring));
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
    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server Started\n");

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("WebSocket server started on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}
