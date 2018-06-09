//
// Created by student on 07.06.18.
//

#ifndef SOCKETS_COMMON_H
#define SOCKETS_COMMON_H

#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <memory.h>

#define RED_COLOR "\e[1;31m"
#define RESET_COLOR "\e[0m"

#ifndef UNIX_PATH_MAX
struct sockaddr_un sizeCheck;
#define UNIX_PATH_MAX sizeof(sizeCheck.sun_path)
#endif

#define FAILURE_EXIT(msg, ...) {                            \
    printf(RED_COLOR msg RESET_COLOR, ##__VA_ARGS__);       \
    exit(EXIT_FAILURE);                                     }

enum operation{
    ADD,
    SUB,
    MUL,
    DIV,
    NAME,
    RESULT,
    ERROR,
    PING,
    PONG
};

struct __attribute__((__packed__)){
    u_int8_t type;
    u_int16_t size;
    int arg1;
    int arg2;
    int counter;
} *expression = NULL;

struct __attribute__((__packed__)) message {
    u_int8_t type;
    u_int16_t size;
    void *content;
};

#endif //SOCKETS_COMMON_H
