#ifndef UTIL_H
#define UTIL_H

#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>

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

#endif