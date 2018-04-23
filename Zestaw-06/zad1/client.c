#include "common.h"

int initClient();
int registerClient();

int serverQid, privateQid;
key_t serverKey, privateKey;

void cleanUp(){
    msgctl(privateQid, IPC_RMID, (struct msqid_ds *) NULL);

    struct msgBuffer queryBuffer;
    queryBuffer.mtype = QUIT;

    errorCode = msgsnd(serverQid, &queryBuffer, MSGBUF_RAW_SIZE, 0);
}

void handleINT(int sig){
    exit(0);
}

int main(int argc, char **argv) {
    errorCode = initClient();
    if(errorCode == -1){
        printf("%s 3\n", strerror(errno));
        exit(errno);
    }

    int myID = registerClient();
    if(myID == -1){
        printf("%s 6\n", strerror(errno));
        exit(errno);
    }

    FILE * inputFile = fopen(argv[1], "r");
    if(inputFile == NULL){
        printf("%s 7\n", strerror(errno));
        return 1;
    }

    char lineBuffer[MAX_LINE_SIZE];
    struct msgBuffer queryBuffer;
    struct msgBuffer responseBuffer;
    char* operations[] = {"MIRROR", "ADD", "SUB", "MUL", "DIV", "TIME", "END"};

    while(fgets(lineBuffer, MAX_LINE_SIZE, inputFile)) {
        int isProperCmd = 0;
        char * ptr = strtok(lineBuffer, " \n\t");
        for (int i = 0; i < sizeof(operations) / sizeof(char *); i++) {
            if (strcmp(operations[i], ptr) == 0) {
                isProperCmd = 1;

                queryBuffer.mtype = MIRROR + i;
                queryBuffer.id = myID;
                ptr = strtok(NULL, "\n");
                if(ptr != NULL) {
                    strcpy(queryBuffer.buffer, ptr);

                }
                else
                    queryBuffer.buffer[0] = '\0';
                if(queryBuffer.buffer[0] != NULL && (queryBuffer.mtype == TIME || queryBuffer.mtype == END)){
                    printf("Syntax of %s is incorrect\n", operations[i]);
                    continue;
                }
                errorCode = msgsnd(serverQid, &queryBuffer, MSGBUF_RAW_SIZE, 0);
                if (errorCode == -1 && errno) {
                    printf("%s 8\n", strerror(errno));
                    return 1;
                }
                break;
            }
        }
        if (!isProperCmd) {
            printf("Given command \'%s\' is incorrect", lineBuffer);
            return 1;
        }
    }


    while(1){
        errorCode = (int)msgrcv(privateQid, &responseBuffer, MSGBUF_RAW_SIZE, REGISTERY, MSG_EXCEPT);
        if(errorCode == -1) {
            printf("%s 9\n", strerror(errno));
            return 1;
        }
        printf("Received msg from server: %s\noutput of query: %s\n\n",
               responseBuffer.buffer,
               operations[responseBuffer.mtype - MIRROR]
        );

    }

}

int initClient(){
    atexit(cleanUp);
    signal(SIGINT, handleINT);

    char *homeEnv = getenv("HOME");
    serverKey = ftok(homeEnv, SERVER_ID);
    if(serverKey == -1)
        return -1;
    privateKey = ftok(homeEnv, getpid());
    if(privateKey == -1)
        return -1;
    serverQid = msgget(serverKey, S_IRWXU | S_IRWXG );
    if(serverQid == -1)
        return -1;
    privateQid = msgget(privateKey, S_IRWXU | S_IRWXG | IPC_CREAT | IPC_EXCL);
    if(privateQid == -1)
        return -1;
    return 0;
}

int registerClient(){
    struct msgBuffer queryBuffer;
    struct msgBuffer responseBuffer;

    queryBuffer.mtype = REGISTERY;
    queryBuffer.key = privateKey;

    errorCode = msgsnd(serverQid, &queryBuffer, MSGBUF_RAW_SIZE, 0);
    if(errorCode == -1)
        return -1;

    errorCode = (int) msgrcv(privateQid, &responseBuffer, MSGBUF_RAW_SIZE, REGISTERY, 0);
    if(errorCode == -1)
        return -1;
    return responseBuffer.id;
}