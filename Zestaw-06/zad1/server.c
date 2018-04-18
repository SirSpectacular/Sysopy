//
// Created by student on 17.04.18.
//
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include "xXxJebaczMatek2003xXx.h"


int main(int argc, char **argv){
    int errorCode;

    char *homeEnv = getenv("HOME");

    key_t serverKey;
    int serverQid;

    serverKey = ftok(homeEnv, SERVER_ID);
    if(serverKey == -1){
        printf("%s\n", strerror(errno));
        return 1;
    }

    serverQid = msgget(serverKey, IPC_CREAT | IPC_EXCL);
    if(serverQid == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    struct clientInfo clients[MAX_CLIENT_NUMBER];
    int clientCounter = 0;

    struct msgBuffer_Key keyBuffer;
    struct msgBuffer_Int idBuffer;
    struct msgBuffer_Querry querryBuffer;

    while(1) {
        errorCode = (int) msgrcv(serverQid, &keyBuffer, sizeof(keyBuffer.key), KEY_ID_EXCHANGE, IPC_NOWAIT);
        if (errorCode == -1) {
            if (errno != ENOMSG) {
                printf("%s\n", strerror(errno));
                return 1;
            }
        }
        else {
            clients[clientCounter].key = keyBuffer.key;
            clients[clientCounter].qid = msgget(clients[clientCounter].key, 0);
            if (clients[clientCounter].qid == -1) {
                printf("%s\n", strerror(errno));
                return 1;
            }

            idBuffer.mtype = KEY_ID_EXCHANGE;
            idBuffer.value = clientCounter;
            do {
                errorCode = msgsnd(clients[clientCounter].qid, &idBuffer, sizeof(int), IPC_NOWAIT);
                if (errorCode == -1 && errno != EAGAIN) {
                    printf("%s\n", strerror(errno));
                    return 1;
                }
            } while (errno == EAGAIN);
            clientCounter++;
        }

        errorCode = (int) msgrcv(serverQid, &querryBuffer, sizeof(querryBuffer.id) + sizeof(querryBuffer.buffer), KEY_ID_EXCHANGE, MSG_EXCEPT | IPC_NOWAIT);
        if (errorCode == -1) {
            if (errno != ENOMSG) {
                printf("%s\n", strerror(errno));
                return 1;
            }
        }
        else {
            switch(querryBuffer.mtype){
                case MIRROR: {
                    reverseString(querryBuffer.buffer, strlen(querryBuffer.buffer));


                    break;
                }
                case ADD:
                    break;
                case SUB:
                    break;
                case MUL:
                    break;
                case DIV:
                    break;
                case TIME:
                    break;
                case END:
                    break;
                default:
                    break;
            }
        }

    }
}

void reverseString(char *string, size_t len){
    char tmp;
    for (int i = 0; i < len / 2; i++) {
        tmp = string[i];
        string[i] = string[len - i];
        string[len - 1] = tmp;
    }
}