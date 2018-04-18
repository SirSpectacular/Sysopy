#define  _GNU_SOURCE

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "xXxJebaczMatek2003xXx.h"

int serverQid, privateQid;
key_t serverKey, privateKey;

void cleanUp(){
    msgctl(privateQid, IPC_RMID, (struct msqid_ds *) NULL);
}

int main(int argc, char **argv) {
    atexit(cleanUp);
    char *homeEnv = getenv("HOME");
    serverKey = ftok(homeEnv, SERVER_ID);
    if(serverKey == -1) {
        printf("%s 1\n", strerror(errno));
        return 1;
    }
    privateKey = ftok(homeEnv, getpid());
    if(privateKey == -1) {
        printf("%s 2\n", strerror(errno));
        return 1;
    }

    serverQid = msgget(serverKey, S_IRWXU | S_IRWXG );
    if(serverQid == -1) {
        printf("%s 3\n", strerror(errno));
        return 1;
    }
    privateQid = msgget(privateKey, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL); //IPC_PRIVATE
    if(privateQid == -1) {
        printf("%s 4\n", strerror(errno));
        return 1;
    }

    struct msgBuffer_Key keyBuffer;

    keyBuffer.mtype = KEY_ID_EXCHANGE;
    keyBuffer.key = privateKey;

    errorCode = msgsnd(serverQid, &keyBuffer, sizeof(keyBuffer) - sizeof(long), 0);
    if(errorCode == -1) {
        printf("%s 5\n", strerror(errno));
        return 1;
    }

    struct msgBuffer_Int idBuffer;


    errorCode = (int) msgrcv(privateQid, &idBuffer, sizeof(idBuffer) - sizeof(long), KEY_ID_EXCHANGE, 0);
    if(errorCode == -1) {
        printf("%s 6\n", strerror(errno));
        return 1;
    }

    int myID = idBuffer.value;

    FILE * inputFile = fopen(argv[1], "r");
    if(inputFile == NULL){
        printf("%s 7\n", strerror(errno));
        return 1;
    }

    char lineBuffer[MAX_LINE_SIZE];
    char* operations[] = {"MIRROR", "ADD", "SUB", "MUL", "DIV", "TIME", "END"};
    struct msgBuffer_Querry querryBuffer;

    while(fgets(lineBuffer, MAX_LINE_SIZE, inputFile)) {
        int isProperCmd = 0;
        char * ptr = strtok(lineBuffer, " \n\t");
        for (int i = 0; i < sizeof(operations) / sizeof(char *); i++) {
            if (strcmp(operations[i], ptr) == 0) {

                isProperCmd = 1;


                querryBuffer.mtype = MIRROR + i;
                querryBuffer.id = myID;
               // ptr += 4;
                ptr = strtok(NULL, "\n");
                strcpy(querryBuffer.buffer, ptr);
                errorCode = msgsnd(serverQid, &querryBuffer, sizeof(querryBuffer) - sizeof(long), IPC_NOWAIT);
                if (errorCode == -1 && errno != EAGAIN) {
                    printf("%s 8\n", strerror(errno));
                    return 1;
                }
            }
        }
        if (!isProperCmd) {
            printf("Given command \'%s\' is incorrect", lineBuffer);
            return 1;
        }
    }

    struct msgBuffer_Querry responseBuffer;
    while(1){
        errorCode = (int)msgrcv(privateQid, &responseBuffer, sizeof(responseBuffer) - sizeof(long), KEY_ID_EXCHANGE, MSG_EXCEPT);
        if(errorCode == -1) {
            printf("%s 9\n", strerror(errno));
            return 1;
        }
        printf("%s\n", responseBuffer.buffer);
    }
    //NO MSGES
    return 0;
}