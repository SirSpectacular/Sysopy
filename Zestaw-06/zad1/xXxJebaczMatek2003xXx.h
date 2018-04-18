//
// Created by student on 17.04.18.
//

#ifndef IPC_XXXJEBACZMATEK2003XXX_H
#define IPC_XXXJEBACZMATEK2003XXX_H

#define MAX_CLIENT_NUMBER 1024
#define MAX_LINE_SIZE 512

#define SERVER_ID 's'

enum OperationTypes{
    KEY_ID_EXCHANGE = 1,
    MIRROR,
    ADD,
    SUB,
    MUL,
    DIV,
    TIME,
    END
};

struct msgBuffer_Key {
    long mtype;
    key_t key;
};
struct msgBuffer_Int {
    long mtype;
    int value;
};
struct msgBuffer_Querry {
    long mtype;
    int id;
    char buffer[MAX_LINE_SIZE];
};

struct msgBuffer_String {
    long mtype;
    char buffer[MAX_LINE_SIZE];
};


struct clientInfo {
    key_t key;
    int qid;
};

int errorCode;

#endif //IPC_XXXJEBACZMATEK2003XXX_H
