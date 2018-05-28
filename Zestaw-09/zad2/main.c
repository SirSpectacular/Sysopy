#define _POSIX_C_SOURCE 200809L
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <bits/time.h>
#include <sys/time.h>
#include <bits/sigthread.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

#define RED_COLOR "\e[1;31m"
#define RESET_COLOR "\e[0m"

#define MAX_LINE_LEN 4096

#define FAILURE_EXIT(msg, ...) {                            \
    printf(RED_COLOR msg RESET_COLOR, ##__VA_ARGS__);       \
    exit(EXIT_FAILURE);                                     }
int errorCode = 0;

struct procProperties {
    size_t amountOfProducers;                     //P
    size_t amountOfConsumers;                     //K
    size_t sizeOfBuffer;                          //N

    FILE* inputFile;
    int compareValue;                             //L
    int (*compareFunction)(char*);                //mode of searching

    int displayMode;
    int timer;                                    //nk
} properties;

char **theBuffer;
int currentProducer;
int currentConsumer;

int reachedEOF = 0;

sem_t *cellMutexes;
sem_t supervisorMutex;
sem_t outMutex;

pthread_t mainThreadId;
pthread_t *producers;
pthread_t *consumers;

void initProgram(char*);
void cleanUpProgram();
int intFromFile(FILE*, int*);
int wordFromFile(FILE*, char**);
int stringLonger(char*);
int stringEqual(char*);
int stringShorter(char*);
void* producersRoutine(void*);
void* consumersRoutine(void*);
int getPIndex();
int getCIndex();
void aquireSemaphore(sem_t*);
void releaseSemaphore(sem_t*);

//========================================================================================================================

void handleINT(int sig){
    for(int i = 0; i < properties.amountOfProducers; i++)
        pthread_cancel(producers[i]);

    for(int i = 0; i < properties.amountOfConsumers; i++)
        pthread_cancel(consumers[i]);
    exit(0);
}

int main(int argc, char **argv) {
    if(argc != 2) FAILURE_EXIT("Incorrect format of command line arguments")
    initProgram(argv[1]);

    if(properties.timer == 0) {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_handler = handleINT;
        act.sa_flags = 0;
        sigaction(SIGINT, &act, NULL);
    }
    atexit(cleanUpProgram);

    producers = malloc(sizeof(pthread_t) * properties.amountOfProducers);
    consumers = malloc(sizeof(pthread_t) * properties.amountOfConsumers);

    for(int i = 0; i < properties.amountOfProducers; i++)
        pthread_create(&producers[i], NULL, producersRoutine, NULL);

    for(int i = 0; i < properties.amountOfConsumers; i++)
        pthread_create(&consumers[i], NULL, consumersRoutine, NULL);

    if(properties.timer > 0) {
        sleep((unsigned int)properties.timer);
        aquireSemaphore(&outMutex);
        for(int i = 0; i < properties.amountOfProducers; i++)
            pthread_cancel(producers[i]);

        for(int i = 0; i < properties.amountOfConsumers; i++)
            pthread_cancel(consumers[i]);
    } else
        while(1){pause();};

    return 0;
}

//=========================================================================================================================

void initProgram(char *propertiesFilename) {
    //PROGRAM PROPERTIES
    FILE *propertiesFile = fopen(propertiesFilename, "r");
    if (propertiesFile == NULL)
        FAILURE_EXIT("Was unable to open properties file: %s", strerror(errno))

    errorCode = intFromFile(propertiesFile, (int*)&properties.amountOfProducers);
    if (errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 1")

    errorCode = intFromFile(propertiesFile, (int*)&properties.amountOfConsumers);
    if (errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 2")

    errorCode = intFromFile(propertiesFile, (int*)&properties.sizeOfBuffer);
    if (errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 3")

    char *inputFilename;
    errorCode = wordFromFile(propertiesFile, &inputFilename);
    if (errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 4")
    properties.inputFile = fopen(inputFilename, "r");
    if(properties.inputFile == NULL)
        FAILURE_EXIT("Was unable to open input file: %s", strerror(errno))
    free(inputFilename);

    errorCode = intFromFile(propertiesFile, &properties.compareValue);
    if (errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 5")

    char *compareMode;
    errorCode = wordFromFile(propertiesFile, &compareMode);
    if(errorCode != 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 6")
    if (strcmp(compareMode, ">") == 0)
        properties.compareFunction = stringLonger;
    else if (strcmp(compareMode, "<") == 0)
        properties.compareFunction = stringShorter;
    else if (strcmp(compareMode, "=") == 0)
        properties.compareFunction = stringEqual;
    else FAILURE_EXIT("Incorrect mode of searching");
    free(compareMode);

    errorCode = intFromFile(propertiesFile, &properties.displayMode);
    if (errorCode != 0 || (properties.displayMode != 1 && properties.displayMode != 0)) //TODO: Read from string
        FAILURE_EXIT("Was unable to read argument from properties file: 7")

    errorCode = intFromFile(propertiesFile, &properties.timer);
    if (errorCode != 0 || properties.timer < 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 8")

    fclose(propertiesFile);

    //MAIN BUFFER
    theBuffer = malloc(sizeof(char*) * properties.sizeOfBuffer);
    if(theBuffer == NULL)
        FAILURE_EXIT("Was unable to set up buffer: %s", strerror(errno))
    currentConsumer = -1;
    currentProducer = -1;

    //MUTEXES
    cellMutexes = malloc(sizeof(sem_t) * properties.sizeOfBuffer);
    for(int i = 0; i < properties.sizeOfBuffer; i++) {
        errorCode = sem_init(&cellMutexes[i], 0, 1);
        if(errorCode != 0)
            FAILURE_EXIT("Was unable to setup mutexes: %s", strerror(errno))
    }

    errorCode = sem_init(&outMutex, 0, 1);
    if(errorCode != 0)
    FAILURE_EXIT("Was unable to setup mutexes: %s", strerror(errno))

    errorCode = sem_init(&supervisorMutex, 0, 1);
    if(errorCode != 0)
    FAILURE_EXIT("Was unable to setup mutexes: %s", strerror(errno))

    //MAIN THREAD ID
    mainThreadId = pthread_self();
}

int intFromFile(FILE *file, int *out) {
    char buffer[16];
    char *dump;
    errorCode = fscanf(file, "%s", buffer);
    if(errorCode != 1)
        return -1;
    *out = (int)strtol(buffer, &dump, 10);
    if(*dump != '\0')
        return -1;
    return 0;
}

int wordFromFile(FILE *file, char **out) {
    char buffer[4096];
    errorCode = fscanf(file, "%s", buffer);
    if(errorCode != 1)
        return -1;
    *out = malloc(sizeof(char)*strlen(buffer));
    if(*out == NULL)
        return -1;
    strcpy(*out, buffer);
    return 0;
}

void cleanUpProgram() {
    free(theBuffer);
    fclose(properties.inputFile);


    cellMutexes = malloc(sizeof(sem_t*) * properties.sizeOfBuffer);
    for(int i = 0; i < properties.sizeOfBuffer; i++) {
        sem_destroy(&cellMutexes[i]);
    }

    sem_destroy(&outMutex);
    sem_destroy(&supervisorMutex);
}

int stringLonger(char *str) {
    return strlen(str) > properties.compareValue ? 1 : 0;
}
int stringEqual(char *str) {
    return strlen(str) == properties.compareValue ? 1 : 0;
}
int stringShorter(char *str) {
    return strlen(str) < properties.compareValue ? 1 : 0;
}

void* producersRoutine(void* args){
    int index;
    char lineBuffer[MAX_LINE_LEN];
    pthread_t myId = pthread_self();

    while(1) {
      /*  if(properties.displayMode) printf("%lo Producer: Locked file mutex\n", myId);

        if(properties.displayMode) printf("%lo Producer: Unlocked file mutex\n", myId);*/

        aquireSemaphore(&supervisorMutex);
        if(properties.displayMode) printf("%lo Producer: Locked pointer mutex\n", myId);
        index = getPIndex();
        if(properties.displayMode) printf("%lo Producer: Computed my place to work at: %d\n", myId, index);

        if (fgets(lineBuffer, MAX_LINE_LEN, properties.inputFile) == NULL) { //I used static buffer, and fgets() instead of getline(), because I want producers to hold file for as little time as possible.
            if(properties.displayMode) printf("%lo Producer: Reached EOF\n", myId);
            if (properties.timer == 0) reachedEOF = 1;
            currentProducer--;
            releaseSemaphore(&supervisorMutex);
            if(properties.displayMode) printf("%lo Producer: Unlocked pointer mutex\n", myId);
            pause();
        }
        if(properties.displayMode) printf("%lo Producer: Read line from file\n", myId);

        aquireSemaphore(&cellMutexes[index]);
        if(properties.displayMode) printf("%lo Producer: Locked cell[%d] mutex\n", myId, index);

        releaseSemaphore(&supervisorMutex);
        if(properties.displayMode) printf("%lo Producer: Unlocked pointer mutex\n", myId);

        theBuffer[index] = malloc(sizeof(char) * (strlen(lineBuffer) + 1));
        strcpy(theBuffer[index], lineBuffer);
        lineBuffer[0] = 0;
        if(properties.displayMode) printf("%lo Producer: Allocated line in the buffer\n", myId);

        releaseSemaphore(&cellMutexes[index]);
        if(properties.displayMode) printf("%lo Producer: Unlocked cell[%d] mutex\n", myId, index);
    }
}

void* consumersRoutine(void* args) {
    int index;
    pthread_t myId = pthread_self();

    while(1){
        aquireSemaphore(&supervisorMutex);
        if(properties.displayMode) printf("%lo Consumer: Locked pointer mutex\n", myId);

        index = getCIndex();
        if(properties.displayMode) printf("%lo Consumer: Computed my place to work at: %d\n", myId, index);
        aquireSemaphore(&cellMutexes[index]);
        if(properties.displayMode) printf("%lo Consumer: Locked cell[%d] mutex\n", myId, index);

        releaseSemaphore(&supervisorMutex);
        if(properties.displayMode) printf("%lo Consumer: Unlocked pointer mutex\n", myId);

        if (properties.compareFunction(theBuffer[index])) {
            if(theBuffer[index][strlen(theBuffer[index]) - 1] == '\n')
                theBuffer[index][strlen(theBuffer[index]) - 1] = '\0';
            aquireSemaphore(&outMutex);
            printf("[%d]: \"%s\"\n", index, theBuffer[index]);
            releaseSemaphore(&outMutex);
        }
        free(theBuffer[index]);
        theBuffer[index] = NULL;
        if(properties.displayMode) printf("%lo Consumer: Freed line in the buffer\n", myId);
        if(properties.timer == 0 && reachedEOF == 1 && currentConsumer == currentProducer) pthread_kill(mainThreadId, SIGINT);

        releaseSemaphore(&cellMutexes[index]);
        if(properties.displayMode) printf("%lo Consumer: Unlocked cell[%d] mutex\n", myId, index);
    }
}


int getPIndex(){
    while(currentProducer + 1 == currentConsumer || (currentProducer == properties.sizeOfBuffer - 1 && (currentConsumer == 0 || currentConsumer == -1))) { //while is full
        releaseSemaphore(&supervisorMutex);
        aquireSemaphore(&supervisorMutex);
    }

    if(currentProducer == properties.sizeOfBuffer - 1)
        currentProducer = 0;
    else
        currentProducer++;
    return currentProducer;
}

int getCIndex(){
    while(currentConsumer == currentProducer) {// while is empty
        releaseSemaphore(&supervisorMutex);
        aquireSemaphore(&supervisorMutex);
        if(properties.timer == 0 && reachedEOF == 1) pthread_kill(mainThreadId, SIGINT);
    }

    if(currentConsumer == properties.sizeOfBuffer - 1)
        currentConsumer = 0;
    else
        currentConsumer++;
    return currentConsumer;

}

void aquireSemaphore(sem_t *semID){
    errorCode = sem_wait(semID);
    if (errorCode == -1) {
        printf("%s 3\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void releaseSemaphore(sem_t *semID){
    errorCode = sem_post(semID);
    if (errorCode == -1) {
        printf("%s 4\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
