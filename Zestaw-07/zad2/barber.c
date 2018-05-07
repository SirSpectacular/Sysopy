#include "common.h"

//
// Created by student on 24.04.18.
//

int errorCode;
int shmID = 0;
sem_t *semID = 0;

void aquireSemaphore(sem_t*);
void releaseSemaphore(sem_t*);
int initBarber(int*, int, char**);
int initBarbershop(int);
int isQueueEmpty();
void inviteClient();
void serveClient(sem_t*);


void handleSIGTERM(int sig){
    exit(0);
}

void cleanUp() {
    if(semID != 0) {
        sem_close(semID);
    }
    if(shmID != 0){
        shm_unlink(PROJECT_PATH);
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
            case AWAKEN:
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
    shmID = shm_open(PROJECT_PATH, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG);
    if (shmID == -1){
        printf("%s 21\n", strerror(errno));
        return -1;
    }

    errorCode = ftruncate(shmID, sizeof(*barbershop));
    if (errorCode == -1){
        printf("%s 22\n", strerror(errno));
        return -1;
    }
    barbershop = mmap(NULL, sizeof(*barbershop), PROT_READ | PROT_WRITE, MAP_SHARED, shmID, 0);
    if(barbershop == (void*)-1){
        printf("%s 23\n", strerror(errno));
        return -1;
    }
    semID = sem_open(PROJECT_PATH, O_WRONLY | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG, 0);
    if (semID == (void*)-1){
        printf("%s 24\n", strerror(errno));
        return -1;
    }

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

void serveClient(sem_t *semID){
    printf("%lo: Started shaving client %d\n", getTimestamp(), barbershop->selectedClient);
    /*releaseSemaphore(semID);
    sleep(1);
    aquireSemaphore(semID);*/
    printf("%lo: Finished shaving client %d\n", getTimestamp(), barbershop->selectedClient);
    barbershop->selectedClient = 0;
}


