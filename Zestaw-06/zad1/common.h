//
// Created by student on 17.04.18.
//
#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define MAX_CLIENT_NUMBER 1024
#define MAX_LINE_SIZE 512

#define SERVER_ID 's'

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


struct msgBuffer {
    long mtype;
    int id;
    key_t key;
    char buffer[MAX_LINE_SIZE];
};

const size_t MSGBUF_RAW_SIZE = sizeof(struct msgBuffer) - sizeof(long);

struct clientInfo {
    int qid;
};

int errorCode;

#endif //COMMON_H
