//
// Created by student on 17.04.18.
//

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
#include "xXxJebaczMatek2003xXx.h"

key_t serverKey;
int serverQid;

void reverseString(char *dest, char *source) {
    size_t len = strlen(source);
    for (int i = 0; i < len; i++) {
        dest[i] = source[len - i - 1];
    }
    dest[len] = '\0';
}

int stringToInt(int *dest, char *source) {
    char *dump;
    *dest = (int) strtol(source, &dump, 10);
    if(*dump != '\0') return -1;
    return 0;
}

void cleanUp(){
    msgctl(serverQid, IPC_RMID, (struct msqid_ds *) NULL);
}

int main(int argc, char **argv) {

    atexit(cleanUp);

    char *homeEnv = getenv("HOME");

    serverKey = ftok(homeEnv, SERVER_ID);
    if (serverKey == -1) {
        printf("%s 1\n", strerror(errno));
        return 1;
    }

    serverQid = msgget(serverKey, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL);
    if (serverQid == -1) {
        printf("%s 2\n", strerror(errno));
        return 1;
    }

    struct clientInfo clients[MAX_CLIENT_NUMBER];
    int clientCounter = 0;

    struct msgBuffer_Key keyBuffer;
    struct msgBuffer_Int idBuffer;
    struct msgBuffer_Querry querryBuffer;
    while (1) {
        errorCode = (int) msgrcv(serverQid, &keyBuffer, sizeof(keyBuffer) - sizeof(long), KEY_ID_EXCHANGE, IPC_NOWAIT);
        if (errorCode == -1) {
            if (errno != ENOMSG) {
                printf("%s 3\n", strerror(errno));
                return 1;
            }
        } else {
            //printf("Got 1: %d - key %lo - type\n", keyBuffer.key, keyBuffer.mtype );

            clients[clientCounter].key = keyBuffer.key;
            clients[clientCounter].qid = msgget(clients[clientCounter].key, S_IRWXU | S_IRWXG);
            if (clients[clientCounter].qid == -1) {
                printf("%s 4\n", strerror(errno));
                return 1;
            }


            idBuffer.mtype = KEY_ID_EXCHANGE;
            idBuffer.value = clientCounter;
            errorCode = msgsnd(clients[clientCounter].qid, &idBuffer, sizeof(idBuffer) - sizeof(long), IPC_NOWAIT);
            if (errorCode == -1) {
                printf("%s 5\n", strerror(errno));
                return 1;
            }
           // printf("Sent id: %d - id %lo - type\n", idBuffer.value, idBuffer.mtype);
            clientCounter++;
        }

        errorCode = (int) msgrcv(serverQid, &querryBuffer, sizeof(querryBuffer) - sizeof(long), KEY_ID_EXCHANGE, MSG_EXCEPT | IPC_NOWAIT);
        if (errorCode == -1) {
            if (errno != ENOMSG) {
                printf("%s 6\n", strerror(errno));
                return 1;
            }
        }
        else {
            struct msgBuffer_String responseBuffer;
            printf("Got querry %s - msg %d - id %lo - type\n", querryBuffer.buffer, querryBuffer.id, querryBuffer.mtype);
            switch (querryBuffer.mtype) {
                case KEY_ID_EXCHANGE:
                case MIRROR: {
                    reverseString(responseBuffer.buffer, querryBuffer.buffer);
                    break;
                }
                case ADD: {
                    int num1, num2;
                    errorCode = stringToInt(&num1, strtok(querryBuffer.buffer, " \n\t"));
                    errorCode |= stringToInt(&num2, strtok(NULL, " \n\t"));
                    if (errorCode != 0 || strtok(NULL, " \n\t") != NULL)
                        sprintf(responseBuffer.buffer, "Wrong syntax of ADD querry from client no. %d\n",
                                querryBuffer.id);
                    else
                        sprintf(responseBuffer.buffer, "%d\n", num1 + num2);
                    break;
                }
                case SUB: {
                    int num1, num2;
                    errorCode = stringToInt(&num1, strtok(querryBuffer.buffer, " \n\t"));
                    errorCode |= stringToInt(&num2, strtok(NULL, " \n\t"));
                    if (errorCode != 0 || strtok(NULL, " \n\t") != NULL)
                        sprintf(responseBuffer.buffer, "Wrong syntax of SUB querry from client no. %d\n",
                                querryBuffer.id);
                    else
                        sprintf(responseBuffer.buffer, "%d\n", num1 - num2);
                    break;
                }
                case MUL: {
                    int num1, num2;
                    errorCode = stringToInt(&num1, strtok(querryBuffer.buffer, " \n\t"));
                    errorCode |= stringToInt(&num2, strtok(NULL, " \n\t"));
                    if (errorCode != 0 || strtok(NULL, " \n\t") != NULL)
                        sprintf(responseBuffer.buffer, "Wrong syntax of MUL querry from client no. %d\n",
                                querryBuffer.id);
                    else
                        sprintf(responseBuffer.buffer, "%d\n", num1 * num2);
                    break;
                }
                case DIV: {
                    int num1, num2;
                    errorCode = stringToInt(&num1, strtok(querryBuffer.buffer, " \n\t"));
                    errorCode |= stringToInt(&num2, strtok(NULL, " \n\t"));
                    if (errorCode != 0 || strtok(NULL, " \n\t") != NULL)
                        sprintf(responseBuffer.buffer, "Wrong syntax of MUL querry from client no. %d\n",
                                querryBuffer.id);
                    else
                        sprintf(responseBuffer.buffer, "%d\n", num1 * num2);
                    break;
                }
                case TIME: {
                    struct timeval timeBuffer;
                    struct tm *formatedTime;
                    gettimeofday(&timeBuffer, NULL);
                    formatedTime = localtime(&timeBuffer.tv_sec);
                    strftime(responseBuffer.buffer, sizeof(responseBuffer.buffer), "%Y-%m-%d %H:%M:%S", formatedTime);
                    break;
                }
                case END: {
                    exit(0);
                }
                default:
                    break;
            }
            responseBuffer.mtype = querryBuffer.mtype;
            errorCode = msgsnd(clients[querryBuffer.id].qid, &responseBuffer, sizeof(responseBuffer) - sizeof(long), 0);
            if (errorCode == -1) {
                for(int i = 0; i < 10; i++)
                printf("%s 7\n", strerror(errno));
                return 1;
            }
            printf("Sent response: %s - msg %lo - type\n", responseBuffer.buffer, responseBuffer.mtype);
        }
    }
}
