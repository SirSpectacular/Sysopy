#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>

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
    int nk;
} properties;

char **theBuffer;
int currentProducer;
int currentConsumer;

pthread_mutex_t* cellMutexes;
pthread_mutex_t pointerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t notFull = PTHREAD_COND_INITIALIZER;
pthread_cond_t notEmpty = PTHREAD_COND_INITIALIZER;

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

//========================================================================================================================

int main(int argc, char **argv) {
    if(argc != 2) FAILURE_EXIT("Incorrect format of command line arguments")
    initProgram(argv[1]);

    pthread_t *producers = malloc(sizeof(pthread_t) * properties.amountOfProducers);
    pthread_t *consumers = malloc(sizeof(pthread_t) * properties.amountOfConsumers);

    for(int i = 0; i < properties.amountOfProducers; i++)
        pthread_create(&producers[i], NULL, producersRoutine, NULL);

    for(int i = 0; i < properties.amountOfConsumers; i++)
        pthread_create(&consumers[i], NULL, consumersRoutine, NULL);

    if(properties.nk > 0) {
        sleep((unsigned int) properties.nk);
    }
    cleanUpProgram();
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

    errorCode = intFromFile(propertiesFile, &properties.nk);
    if (errorCode != 0 || properties.nk < 0)
        FAILURE_EXIT("Was unable to read argument from properties file: 8")

    fclose(propertiesFile);

    //MAIN BUFFER
    theBuffer = malloc(sizeof(char*) * properties.sizeOfBuffer);
    if(theBuffer == NULL)
        FAILURE_EXIT("Was unable to set up buffer: %s", strerror(errno))
    currentConsumer = -1;
    currentProducer = -1;

    //MUTEXES
    cellMutexes = malloc(sizeof(pthread_mutex_t) * properties.sizeOfBuffer);
    for(int i = 0; i < properties.sizeOfBuffer; i++) {
        errorCode = pthread_mutex_init(&cellMutexes[i], NULL);
        if(errorCode != 0)
            FAILURE_EXIT("Was unable to setup mutexes: %s", strerror(errno))
    }
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
    while(1){
        pthread_mutex_lock(&pointerMutex);
        index = getPIndex();
        pthread_mutex_lock(&cellMutexes[index]);
        pthread_mutex_unlock(&pointerMutex);

        pthread_mutex_lock(&fileMutex);
        if(fgets(lineBuffer, MAX_LINE_LEN, properties.inputFile) == NULL) //I used static buffer, and fgets() instead of getline(), because I want producers to hold file for as little time as possible.
            lineBuffer[0] = 0;
        pthread_mutex_unlock(&fileMutex);

        theBuffer[index] = malloc(sizeof(char) * (strlen(lineBuffer) + 1));
        strcpy(theBuffer[index], lineBuffer);
        pthread_mutex_unlock(&cellMutexes[index]);
    }
}

void* consumersRoutine(void* args) {
    int index;

    while(1){
        pthread_mutex_lock(&pointerMutex);
        index = getCIndex();
        pthread_mutex_lock(&cellMutexes[index]);
        pthread_mutex_unlock(&pointerMutex);

        if (properties.compareFunction(theBuffer[index])) {

            printf("[%d]: %s", index, theBuffer[index]);
        }
        free(theBuffer[index]);
        theBuffer[index] = NULL;

        pthread_mutex_unlock(&cellMutexes[index]);
    }
}


int getPIndex(){
    while(currentProducer + 1 == currentConsumer || (currentProducer == properties.sizeOfBuffer - 1 && currentConsumer == 0)) //while is full
        pthread_cond_wait(&notFull, &pointerMutex);
    if(currentConsumer == currentProducer)
        pthread_cond_broadcast(&notEmpty);

    if(currentProducer == properties.sizeOfBuffer - 1)
        currentProducer = 0;
    else
        currentProducer++;
    return currentProducer;
}

int getCIndex(){
    while(currentConsumer == currentProducer) // while is empty
        pthread_cond_wait(&notEmpty, &pointerMutex);
    if(currentProducer + 1 == currentConsumer || (currentProducer == properties.sizeOfBuffer - 1 && currentConsumer == 0))
        pthread_cond_broadcast(&notFull);

    if(currentConsumer == properties.sizeOfBuffer - 1)
        currentConsumer = 0;
    else
        currentConsumer++;
    return currentConsumer;

}
