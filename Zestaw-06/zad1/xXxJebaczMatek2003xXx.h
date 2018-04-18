//
// Created by student on 17.04.18.
//

#ifndef IPC_XXXJEBACZMATEK2003XXX_H
#define IPC_XXXJEBACZMATEK2003XXX_H

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
    ERROR
};

char* operations[] = {"MIRROR", "ADD", "SUB", "MUL", "DIV", "TIME", "END"};

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

#endif //IPC_XXXJEBACZMATEK2003XXX_H
