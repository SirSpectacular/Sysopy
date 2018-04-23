//
// Created by student on 19.04.18.
//


#include "common.h"

int initServer();
void cleanUp();
void handleINT(int);
int handleREGISTERY(struct msgBuffer, struct msgBuffer*);
void handleMIRROR(struct msgBuffer, struct msgBuffer*);
void handleCALC(struct msgBuffer, struct msgBuffer*, enum operationType);
int stringToInt(int *dest, char *source);
void handleTIME(struct msgBuffer, struct msgBuffer*);
int handleQUIT(struct msgBuffer);


mqd_t serverDesc;
mqd_t clientDescs[MAX_CLIENT_NUMBER];
int clientCounter = 0;

int main(int argc, char **argv){
    serverDesc = initServer();
    if (serverDesc == -1) {
        printf("%s\n", strerror(errno));
        return -1;
    }

    struct msgBuffer queryBuffer;
    struct msgBuffer responseBuffer;
    int readyToExit = 0;

    while(1){
        errorCode = (int)mq_receive(serverDesc, (char*)&queryBuffer, sizeof(queryBuffer), 0);
        if(errorCode == -1) {
            if (errno != EAGAIN) {
                printf("%s\n", strerror(errno));
                return -1;
            } else if(readyToExit == 1) exit(0);
        }
        else{
            printf("Got query %s - msg %d - id %lo - type\n", queryBuffer.buffer, queryBuffer.id, queryBuffer.type);
            switch (queryBuffer.type) {
                case REGISTERY:
                    errorCode = handleREGISTERY(queryBuffer, &responseBuffer);
                    if (errorCode == -1) {
                        printf("%s\n", strerror(errno));
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
                case QUIT:
                    errorCode = handleQUIT(queryBuffer);
                    if (errorCode == -1) {
                        printf("%s\n", strerror(errno));
                        return -1;
                    }
                    continue;
                case END:
                    readyToExit = 1;
                    continue;
                default:
                    continue;
            }
            responseBuffer.type = queryBuffer.type;
            do {
                errorCode = mq_send(clientDescs[responseBuffer.id], (char *) &responseBuffer, sizeof(struct msgBuffer), 0);
                if (errorCode == -1 && errno != EAGAIN) {
                    printf("%s\n", strerror(errno));
                    return -1;
                }
            } while(errorCode == -1);
        }
    }
}

int initServer(){
    atexit(cleanUp);
    signal(SIGINT, handleINT);

    struct mq_attr attr;
    attr.mq_msgsize = sizeof(struct msgBuffer);
    attr.mq_maxmsg = 10;

    return mq_open(SERVER_PATH, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, S_IRWXU | S_IRWXG, &attr);
}

void cleanUp(){
    mq_close(serverDesc);
    mq_unlink(SERVER_PATH);
    for(int i = 0; i < clientCounter; i++)
        mq_close(clientDescs[i]);
}

void handleINT(int sig){
    exit(0);
}


int handleREGISTERY(struct msgBuffer queryBuffer, struct msgBuffer *responseBuffer){
    clientDescs[clientCounter] = mq_open(queryBuffer.buffer, O_WRONLY | O_NONBLOCK);
    if (clientDescs[clientCounter] == -1)
        return -1;
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
    if(mq_close(clientDescs[queryBuffer.id]) == -1)
        return -1;

    for(int i = queryBuffer.id; i < clientCounter; i++)
        clientDescs[i] = clientDescs[i+1];
    clientCounter--;
    return 0;
}

