

#include "common.h"

//
// Created by student on 24.04.18.
//
int initClients(int*, int*, int, char**);
int clientRoutine(sem_t*);
void aquireSemaphore(sem_t*);
void releaseSemaphore(sem_t*);
void claimChair();
void enterQueue();
int isQueueFull();
enum clientStatus updateStatus();
long getTimestamp();

pid_t myPID;
enum clientStatus myStatus;
int shmID;
sem_t *semID;
int errorCode;

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
            int count = 0;
            while(count < S){
                count += clientRoutine(semID);
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

    shmID = shm_open(PROJECT_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if(shmID == -1)
        return -1;
    errorCode = ftruncate(shmID, sizeof(*barbershop));
    if (errorCode == -1)
        return -1;
    barbershop = mmap(NULL, sizeof(*barbershop), PROT_READ | PROT_WRITE, MAP_SHARED, shmID, 0);
    if(barbershop == (void*)-1)
        return -1;

    semID = sem_open(PROJECT_PATH, O_WRONLY, S_IRWXU | S_IRWXG, 0);
    if (semID == (void*)-1)
        return -1;

    return 0;

}

int clientRoutine(sem_t *semID){
    myStatus = NEWCOMER;

    aquireSemaphore(semID);
    if(barbershop->barberStatus == ASLEEP) {
        printf("%lo: %d: Woke up the barber\n", getTimestamp(), myPID);
        barbershop->barberStatus = AWOKEN;
        claimChair();
        barbershop->barberStatus = BUSY;
        }
    else {
        if(!isQueueFull()) {
            enterQueue();
            printf("%lo: %d: Entered the queue\n", getTimestamp(), myPID);
        }
        else {
            printf("%lo: %d: There was not enough space in the queue\n", getTimestamp(), myPID);
            releaseSemaphore(semID);
            return 0;
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
        if((myStatus = updateStatus()) == SHAVED){
            printf("%lo: %d: Got shaved and left\n", getTimestamp(), myPID);
            barbershop->barberStatus = IDLE;
        }
        releaseSemaphore(semID);
    }
    return 1;
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
            if (barbershop->barberStatus == WAIT)
                break;
        }
        myStatus = INVITED;
        barbershop->selectedClient = myPID;
    }
    printf("%lo: %d: Claimed chair\n", getTimestamp(), myPID);
}

void aquireSemaphore(sem_t *semID){
    errorCode = sem_wait(semID);
    if (errorCode == -1) {
        printf("%s 3\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void releaseSemaphore(sem_t *semID) {
    errorCode = sem_post(semID);
    if (errorCode == -1) {
        printf("%s 4\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int isQueueFull() {
    if (barbershop->firstClientIndex == 0 &&
        barbershop->lastClientIndex == barbershop->waitingRoomSize - 1)
        return 1;

    if (barbershop->firstClientIndex - 1 == barbershop->lastClientIndex) return 1;

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

long getTimestamp(){
    struct timespec buf;
    clock_gettime(CLOCK_MONOTONIC, &buf);
    return buf.tv_nsec / 1000;
}