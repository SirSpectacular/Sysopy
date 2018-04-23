//
// Created by student on 17.04.18.
//
#include "common.h"

void cleanUp();
void handleINT(int);
int initServer();
int handleREGISTERY(struct msgBuffer, struct msgBuffer*);
void handleMIRROR(struct msgBuffer, struct msgBuffer*);
void handleCALC(struct msgBuffer, struct msgBuffer*, enum operationType);
int stringToInt(int *dest, char *source);
void handleTIME(struct msgBuffer, struct msgBuffer*);
int handleQUIT(struct msgBuffer);

key_t serverKey;
int serverQid;


int clientQids[MAX_CLIENT_NUMBER];
int clientCounter = 0;

int main(int argc, char **argv) {
    errorCode = initServer();
    if (errorCode == -1) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    while (1) {
        struct msgBuffer queryBuffer;
        struct msgBuffer responseBuffer;
        int readyToExit;

        errorCode = (int) msgrcv(serverQid, &queryBuffer, MSGBUF_RAW_SIZE, 0, IPC_NOWAIT);
        if (errorCode == -1) {
            if (errno != ENOMSG) {
                printf("%s s\n", strerror(errno));
                return 1;
            } else if (readyToExit == 1) exit(0);
        } else {
            printf("Got query: %s - msg,\t %d - id,\t %lo - type\n", queryBuffer.buffer == NULL ? "null" : queryBuffer.buffer, queryBuffer.id, queryBuffer.mtype);
            switch (queryBuffer.mtype) {
                case REGISTERY:
                    errorCode = handleREGISTERY(queryBuffer, &responseBuffer);
                    if (errorCode == -1) {
                        printf("%s 2\n", strerror(errno));
                        return -1;
                    }
                    break;
                case MIRROR:
                    handleMIRROR(queryBuffer, &responseBuffer);
                    break;
                case ADD:
                    handleCALC(queryBuffer, &responseBuffer, ADD);
                    break;
                case SUB:
                    handleCALC(queryBuffer, &responseBuffer, SUB);
                    break;
                case MUL:
                    handleCALC(queryBuffer, &responseBuffer, MUL);
                    break;
                case DIV:
                    handleCALC(queryBuffer, &responseBuffer, DIV);
                    break;
                case TIME:
                    handleTIME(queryBuffer, &responseBuffer);
                    break;
                case END:
                    readyToExit = 1;
                    continue;
                case QUIT:
                    handleQUIT(queryBuffer);
                    continue;
                default:
                    continue;
            }
            responseBuffer.mtype = queryBuffer.mtype;
            errorCode = msgsnd(clientQids[queryBuffer.id], &responseBuffer, MSGBUF_RAW_SIZE, 0);
            if (errorCode == -1) {
                printf("%s\n", strerror(errno));
                return -1;
            }
        }
    }
}


void cleanUp(){
    msgctl(serverQid, IPC_RMID, (struct msqid_ds *) NULL);
}

void handleINT(int sig){
    exit(0);
}

int initServer(){
    atexit(cleanUp);
    signal(SIGINT, handleINT);

    serverKey = ftok(getenv("HOME"), SERVER_ID);
    if (serverKey == -1)
        return -1;

    serverQid = msgget(serverKey, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL);
    if (serverQid == -1)
        return -1;
    return 0;
}

int handleREGISTERY(struct msgBuffer queryBuffer, struct msgBuffer *responseBuffer){
        clientQids[clientCounter] = msgget(queryBuffer.key, S_IRWXU | S_IRWXG);
        if (clientQids[clientCounter] == -1)
            return -1;
        responseBuffer->mtype = REGISTERY;
        responseBuffer->id = clientCounter;
        clientCounter++;
}

void handleMIRROR(struct msgBuffer queryBuffer, struct msgBuffer *responseBuffer){
    size_t len = strlen(queryBuffer.buffer);
    for (int i = 0; i < len; i++) {
        responseBuffer->buffer[i] = queryBuffer.buffer[len - i - 1];
    }
    responseBuffer->buffer[len] = '\0';
}

void handleCALC(struct msgBuffer queryBuffer, struct msgBuffer *responseBuffer, enum operationType type) {
    int num1, num2;
    errorCode = stringToInt(&num1, strtok(queryBuffer.buffer, " \n\t"));
    errorCode |= stringToInt(&num2, strtok(NULL, " \n\t"));
    if (errorCode != 0 || strtok(NULL, " \n\t") != NULL)
        sprintf(responseBuffer->buffer, "Wrong syntax of %s query from client no. %d",
                (type == ADD) ? "ADD" :
                (type == SUB) ? "SUB" :
                (type == MUL) ? "MUL" :
                "DIV",
                queryBuffer.id
        );
    else
        sprintf(responseBuffer->buffer, "%d",
                (type == ADD) ? num1 + num2 :
                (type == SUB) ? num1 - num2 :
                (type == MUL) ? num1 * num2 :
                num1 / num2
        );
}

int stringToInt(int *dest, char *source) {
    char *dump;
    *dest = (int) strtol(source, &dump, 10);
    if(*dump != '\0') return -1;
    return 0;
}

void handleTIME(struct msgBuffer queryBuffer, struct msgBuffer *responseBuffer) {
    if (*queryBuffer.buffer != '\0') {
        sprintf(responseBuffer->buffer, "Wrong syntax of TIME query from client no. %d", queryBuffer.id);
        return;
    }
    struct timeval timeBuffer;
    struct tm *formatedTime;
    gettimeofday(&timeBuffer, NULL);
    formatedTime = localtime(&timeBuffer.tv_sec);
    strftime(responseBuffer->buffer, sizeof(responseBuffer->buffer), "%Y-%m-%d %H:%M:%S", formatedTime);
}

int handleQUIT(struct msgBuffer queryBuffer){
    if(msgctl(clientQids[queryBuffer.id], IPC_RMID, (struct msqid_ds *) NULL) == -1)
        return -1;

    for(int i = queryBuffer.id; i < clientCounter; i++)
        clientQids[i] = clientQids[i+1];
    clientCounter--;
    return 0;
}