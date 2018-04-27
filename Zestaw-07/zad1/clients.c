
#include "common.h"

//
// Created by student on 24.04.18.
//
int initClients(int*, int*, int, char**);
void clientRoutine(int);
void aquireSemaphore(int);
void releaseSemaphore(int);
void claimChair();
void enterQueue();
int isQueueFull();
enum clientStatus updateStatus();

pid_t myPID;
enum clientStatus myStatus;
int shmID;
int semID;
int errorCode;

long getTimestamp(){
    struct timespec buf;
    clock_gettime(CLOCK_MONOTONIC, &buf);
    return buf.tv_nsec / 1000;
}


int main(int argc, char **argv){
    int amountOfClients;
    int S;

    errorCode = initClients(&amountOfClients, &S, argc, argv);
    if(errorCode == -1){
        printf("%s 1\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < amountOfClients; i++) {
        if(!(fork())) {
            myPID = getpid();
            for (int j = 0; j < S; j++) {
                clientRoutine(semID);
            }
            exit(0);
        }
    }
    while(wait(NULL) != -1 && errno != ECHILD);
}

int initClients(int *amountOfClients, int *S, int argc, char **argv){
    if(argc != 3)
        return -1;

    char * dump;
    *amountOfClients = (int) strtol(argv[1], &dump, 10);
    if(*dump != '\0')
        return -1;
    *S = (int) strtol(argv[2], &dump, 10);
    if(*dump != '\0')
        return -1;

    key_t projectKey = ftok(PROJECT_PATH, PROJECT_ID);
    if(projectKey == -1){
        printf("2");
        exit(EXIT_FAILURE);
    }

    shmID = shmget(projectKey, sizeof(struct shmBuffer), 0);
    if(shmID == -1)
        return -1;

    barbershop = shmat(shmID, NULL, 0);
    if(barbershop == (void *)-1)
        return -1;

    semID = semget(projectKey, 0, 0);
    if(semID == -1)
        return -1;

    return 0;

}

void clientRoutine(int semID){
    myStatus = NEWCOMER;

    aquireSemaphore(semID);
    if(barbershop->barberStatus == ASLEEP) {
        printf("%lo: %d: Woke up the barber\n", getTimestamp(), myPID);
        barbershop->barberStatus = AWAKEN;
        claimChair();
        barbershop->barberStatus = BUSY;
        }
    else {
        if(!isQueueFull()) {
            enterQueue();
            printf("%lo: %d: Entered the queue\n", getTimestamp(), myPID);
        }
        else {
            printf("%lo: %d: There was no enough space in the queue\n", getTimestamp(), myPID);
            releaseSemaphore(semID);
            return;
        }
    }
    releaseSemaphore(semID);

    while(myStatus < INVITED) {
        aquireSemaphore(semID);
        if((myStatus = updateStatus()) == INVITED) {
            claimChair();
            barbershop->barberStatus = BUSY;
        }
        releaseSemaphore(semID);
    }

    while(myStatus < SHAVED) {
        aquireSemaphore(semID);
        if((myStatus = updateStatus()) == SHAVED)
            printf("%lo: %d: Shaved\n", getTimestamp(), myPID);
        releaseSemaphore(semID);
    }
}

enum clientStatus updateStatus(){
    if(barbershop->selectedClient == myPID) return INVITED;
    if(myStatus == INVITED && barbershop->selectedClient != myPID) return SHAVED;
    return NEWCOMER;
}

void claimChair(){
    if(myStatus == INVITED) {
        if (barbershop->firstClientIndex == barbershop->lastClientIndex)
            barbershop->firstClientIndex = barbershop->lastClientIndex = -1;
        else if (barbershop->firstClientIndex == barbershop->waitingRoomSize - 1)
            barbershop->firstClientIndex = 0;
        else
            barbershop->firstClientIndex++;
    }
    else if(myStatus == NEWCOMER) {
        while (1) {
            releaseSemaphore(semID);
            aquireSemaphore(semID);
            if (barbershop->barberStatus == READY)
                break;
        }
        myStatus = INVITED;
        barbershop->selectedClient = myPID;
    }
    printf("%lo: %d: Claimed chair\n", getTimestamp(), myPID);
}

void aquireSemaphore(int semID){
    struct sembuf semaphoreRequest;
    semaphoreRequest.sem_num = 0;
    semaphoreRequest.sem_op = -1;
    semaphoreRequest.sem_flg = 0;

    errorCode = semop(semID, &semaphoreRequest, 1);
    if (errorCode == -1) {
        printf("%s 3\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

}

void releaseSemaphore(int semID){
    struct sembuf semaphoreRequest;
    semaphoreRequest.sem_num = 0;
    semaphoreRequest.sem_op = 1;
    semaphoreRequest.sem_flg = 0;

    errorCode = semop(semID, &semaphoreRequest, 1);
    if (errorCode == -1) {
        printf("%s 4\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int isQueueFull(){
    if(barbershop->firstClientIndex == 0 &&
       barbershop->lastClientIndex == barbershop->waitingRoomSize - 1) return 1;

    if(barbershop->firstClientIndex - 1 == barbershop->lastClientIndex) return 1;

    return 0;
}

void enterQueue(){
    if (barbershop->firstClientIndex == -1 && barbershop->lastClientIndex == -1) {
        barbershop->firstClientIndex = barbershop->lastClientIndex = 0;
    }
    else if (barbershop->lastClientIndex == barbershop->waitingRoomSize - 1) {
        barbershop->lastClientIndex = 0;
    }
    else
        barbershop->lastClientIndex++;

    barbershop->queue[barbershop->lastClientIndex] = myPID;
}