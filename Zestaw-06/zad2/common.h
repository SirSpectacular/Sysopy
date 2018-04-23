//
// Created by student on 19.04.18.
//

#ifndef PCOMMON_H
#define PCOMMON_H

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#include <unistd.h>

#define SERVER_PATH "/server"

#define MAX_LINE_SIZE 512
#define MAX_CLIENT_NUMBER 1024
#define MAX_MSG_NUMBER 10

struct msgBuffer {
    long type;
    int id;
    char buffer[MAX_LINE_SIZE];
};

enum operationType{
    REGISTERY = 1,
    MIRROR,
    ADD,
    SUB,
    MUL,
    DIV,
    TIME,
    END,
    QUIT
};

int errorCode;

#endif //COMMON_H
