

#include <sys/time.h>
#include <time.h>
#include "common.h"

//
// Created by student on 24.04.18.
//

int errorCode;
int shmID = 0;
int semID = 0;

void aquireSemaphore(int);
void releaseSemaphore(int);
int initBarber(int*, int, char**);
int initBarbershop(int);
int isQueueEmpty();
void inviteClient();
void serveClient(int);


void handleSIGTERM(int sig){
    exit(0);
}

void cleanUp() {
    if(semID != 0) {
        semctl(semID, 0, IPC_RMID);
    }
    if(shmID != 0){
        shmctl(shmID, IPC_RMID, NULL);
    }
 }

long getTimestamp(){
    struct timespec buf;
    clock_gettime(CLOCK_MONOTONIC, &buf);
    return buf.tv_nsec / 1000;
}

int main(int argc, char **argv) {
    int watingRoomSize;
    errorCode = initBarber(&watingRoomSize, argc, argv);
    if(errorCode == -1){
        printf("%s 1\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    errorCode = initBarbershop(watingRoomSize);
    if(errorCode == -1){
        printf("%s 2\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    releaseSemaphore(semID);

    while (1) {
        aquireSemaphore(semID);
        switch (barbershop->barberStatus) {
            case IDLE:
                if (!isQueueEmpty()) {
                    inviteClient();
                    barbershop->barberStatus = READY;
                }
                else {
                    printf("%lo: Fell asleep\n", getTimestamp());
                    barbershop->barberStatus = ASLEEP;
                }
                break;
            case AWOKEN:
                printf("%lo: Woke up\n", getTimestamp());
                barbershop->barberStatus = READY;
                break;
            case BUSY:
                serveClient(semID);
                barbershop->barberStatus = IDLE;
                break;
            default:
                break;
        }
        releaseSemaphore(semID);
    }

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

int initBarber(int *waitingRoomSize, int argc, char **argv){
    if (argc != 2)
        return -1;

    char *dump;
    *waitingRoomSize = (int) strtol(argv[1], &dump, 10);
    if (*dump != '\0' || *waitingRoomSize > MAX_QUEUE_SIZE)
        return -1;

    signal(SIGTERM, handleSIGTERM);
    signal(SIGINT, handleSIGTERM);
    atexit(cleanUp);

    return 0;
}

int initBarbershop(int waitingRoomSize){
    key_t projectKey = ftok(PROJECT_PATH, PROJECT_ID);
    if (projectKey == -1)
        return -1;

    shmID = shmget(projectKey, sizeof(struct shmBuffer), S_IRWXU | IPC_CREAT);
    if (shmID == -1)
        return -1;
    barbershop = shmat(shmID, NULL, 0);
    if (barbershop == (void *)-1)
        return -1;

    semID = semget(projectKey, 1, IPC_CREAT | S_IRWXU);
    if (semID == -1)
        return -1;


    errorCode = semctl(semID, 0, SETVAL, 0);
    if (errorCode == -1)
        return -1;

    barbershop->barberStatus = ASLEEP;
    barbershop->waitingRoomSize = waitingRoomSize;
    barbershop->firstClientIndex = -1;
    barbershop->lastClientIndex = -1;
    barbershop->selectedClient = 0;

    return 0;
}


int isQueueEmpty(){
    if(barbershop->firstClientIndex == -1 && barbershop->lastClientIndex == -1) return 1;
    return 0;
}

void inviteClient(){
    printf("%lo: Invited client %d\n", getTimestamp(), barbershop->queue[barbershop->firstClientIndex]);
    barbershop->selectedClient = barbershop->queue[barbershop->firstClientIndex];
}

void serveClient(int semID){
    printf("%lo: Started shaving client %d\n", getTimestamp(), barbershop->selectedClient);
    /*releaseSemaphore(semID);
    sleep(1);
    aquireSemaphore(semID);*/
    printf("%lo: Finished shaving client %d\n", getTimestamp(), barbershop->selectedClient);
    barbershop->selectedClient = 0;
}


