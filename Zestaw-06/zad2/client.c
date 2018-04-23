//
// Created by student on 19.04.18.
//



#include "common.h"

int initClient();
int registerClient();
void cleanUp();
void handleINT(int);

mqd_t myDesc;
mqd_t serverDesc;
int myID;

int main(int argc, char **argv) {
    errorCode = initClient();
    if (errorCode == -1) {
        printf("%s 1\n", strerror(errno));
        return 1;
    }
    errorCode = registerClient();
    if (errorCode == -1) {
        printf("%d %s 2\n", errno, strerror(errno));
        return 1;
    }

    FILE *inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        printf("%s 3\n", strerror(errno));
        return 1;
    }

    char lineBuffer[MAX_LINE_SIZE];
    struct msgBuffer queryBuffer;
    struct msgBuffer responseBuffer;
    char *operations[] = {"MIRROR", "ADD", "SUB", "MUL", "DIV", "TIME", "END",};

    while (fgets(lineBuffer, MAX_LINE_SIZE, inputFile)) {
        int isProperCmd = 0;
        char *ptr = strtok(lineBuffer, " \n\t");
        for (int i = 0; i < sizeof(operations) / sizeof(char *); i++) {
            if (strcmp(operations[i], ptr) == 0) {
                isProperCmd = 1;

                queryBuffer.type = MIRROR + i;
                queryBuffer.id = myID;
                ptr = strtok(NULL, "\n");
                if (ptr != NULL) {
                    strcpy(queryBuffer.buffer, ptr);
                } else
                    queryBuffer.buffer[0] = '\0';
                if(queryBuffer.buffer[0] != NULL && (queryBuffer.type == TIME || queryBuffer.type == END)){
                    printf("Syntax of %s is incorrect\n", operations[i]);
                    continue;
                }
                do {
                    errorCode = mq_send(serverDesc, (char *) &queryBuffer, sizeof(struct msgBuffer), 0);
                    if (errorCode == -1 && errno != EAGAIN) {
                        printf("%s \n", strerror(errno));
                        return 1;
                    }
                } while(errorCode == -1);

                break;
            }
        }
        if (!isProperCmd) {
            printf("Given command \'%s\' is incorrect", lineBuffer);
            return 1;
        }
    }


    while (1) {
        errorCode = (int) mq_receive(myDesc, (char*)&responseBuffer, sizeof(struct msgBuffer), 0);
        if (errorCode == -1) {
            if (errno != EAGAIN) {
                printf("%s 9\n", strerror(errno));
                return 1;
            }
        } else {
            printf("Received msg from server: %s\noutput of query: %s\n\n",
                   responseBuffer.buffer,
                   operations[responseBuffer.type - MIRROR]
            );

        }
    }
}

int initClient(){
    atexit(cleanUp);
    signal(SIGINT, handleINT);

    struct mq_attr attr;
    attr.mq_msgsize = sizeof(struct msgBuffer);
    attr.mq_maxmsg = 9;

    char pathBuffer[PATH_MAX];
    sprintf(pathBuffer, "/%d", getpid());

    serverDesc = mq_open(SERVER_PATH, O_WRONLY | O_NONBLOCK);
    if(serverDesc == -1)
        return -1;

    myDesc = mq_open(pathBuffer, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, S_IRWXU | S_IRWXG, &attr);
    if(myDesc == -1)
        return -1;
}

int registerClient(){
    struct msgBuffer queryBuffer;
    struct msgBuffer responseBuffer;

    char pathBuffer[PATH_MAX];
    sprintf(pathBuffer, "/%d", getpid());

    queryBuffer.type = REGISTERY;
    strcpy(queryBuffer.buffer, pathBuffer);

    do {
        errorCode = mq_send(serverDesc, (char *) &queryBuffer, sizeof(struct msgBuffer), 0);
        if (errorCode == -1 && errno != EAGAIN)
            return -1;
    } while(errorCode == -1);

    do {
        errorCode = (int) mq_receive(myDesc, (char*)&responseBuffer, sizeof(struct msgBuffer), 0);
        if (errorCode == -1 && errno != EAGAIN)
            return -1;
    } while(errorCode == -1);

    myID = responseBuffer.id;
    printf("MyID - %d, type - %lo\n", myID, responseBuffer.type);
    return 0;
}


void cleanUp(){
    mq_close(myDesc);

    struct msgBuffer queryBuffer;
    queryBuffer.type = QUIT;
    mq_send(serverDesc, (char*)&queryBuffer, sizeof(struct msgBuffer), 0);

    mq_close(serverDesc);

    char pathBuffer[PATH_MAX];
    sprintf(pathBuffer, "/%d", getpid());
    mq_unlink(pathBuffer);

}

void handleINT(int sig){
    exit(0);
}