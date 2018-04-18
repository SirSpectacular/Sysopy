#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include "xXxJebaczMatek2003xXx.h"

int main(int argc, char **argv) {

    int errorCode;

    char *homeEnv = getenv("HOME");

    key_t serverKey, privateKey;
    int serverQid, privateQid;

    serverKey = ftok(homeEnv, SERVER_ID);
    if(serverKey == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }
    privateKey = ftok(homeEnv, getpid());
    if(privateKey == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    serverQid = msgget(serverKey,0);
    if(serverQid == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }
    privateQid = msgget(privateKey, IPC_CREAT | IPC_EXCL); //IPC_PRIVATE
    if(privateQid == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    struct msgBuffer_Key keyBuffer;

    keyBuffer.mtype = KEY_ID_EXCHANGE;
    keyBuffer.key = privateKey;

    do {
        errorCode = msgsnd(serverQid, &keyBuffer, sizeof(keyBuffer.key), IPC_NOWAIT);
        if(errorCode == -1 && errno != EAGAIN){
            printf("%s\n", strerror(errno));
            return 1;
        }
    } while(errno == EAGAIN);

    struct msgBuffer_Int idBuffer;
    int myID = idBuffer.value;

    errorCode = (int) msgrcv(privateQid, &idBuffer, sizeof(idBuffer.value), KEY_ID_EXCHANGE, 0);
    if(errorCode == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    FILE * inputFile = fopen(argv[1], "r");
    if(inputFile == NULL){
        printf("%s\n", strerror(errno));
        return 1;
    }

    char lineBuffer[MAX_LINE_SIZE];
    char* operations[] = {"MIRROR", "ADD", "SUB", "MUL", "DIV", "TIME", "END"};
    struct msgBuffer_Querry querry;

    while(fgets(lineBuffer, MAX_LINE_SIZE, inputFile)) {
        int isProperCmd = 0;
        for (int i = 0; i < sizeof(operations) / sizeof(char *); i++) {
            if (strcmp(operations[i], strtok(lineBuffer, " \n\t")) == 0) {

                isProperCmd = 1;


                querry.mtype = MIRROR + i;
                querry.id = myID;
                strcpy(querry.buffer, strtok(NULL, "\n"));
                do {
                    errorCode = msgsnd(serverQid, &querry, sizeof(querry.id) + sizeof(querry.buffer), IPC_NOWAIT);
                    if (errorCode == -1 && errno != EAGAIN) {
                        printf("%s\n", strerror(errno));
                        return 1;
                    }
                } while (errno == EAGAIN);
            }
        }
        if (!isProperCmd) {
            printf("Given command \'%s\' is incorrect", lineBuffer);
            return 1;
        }
    }

    struct msgBuffer_Querry response;
    while(1){
        errorCode = (int)msgrcv(privateKey, &response, sizeof(querry.id) + sizeof(querry.buffer), KEY_ID_EXCHANGE, MSG_EXCEPT);
        if(errorCode == -1) {
            printf("%s\n", strerror(errno));
            return 1;
        }
        printf("%s\n", response.buffer);
    }
    //NO MSGES
    return 0;
}