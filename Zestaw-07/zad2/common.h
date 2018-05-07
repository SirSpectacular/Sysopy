//
// Created by student on 24.04.18.
//

#ifndef IPC_SEMAPHORES_SHAREDMEMORY_COMMON_H
#define IPC_SEMAPHORES_SHAREDMEMORY_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_QUEUE_SIZE 512
#define PROJECT_PATH "/barbershop"

enum barberStatus{
    ASLEEP,
    AWAKEN,
    READY,
    IDLE,
    BUSY
};

enum clientStatus{
    NEWCOMER,
    //ENQUEUED
    INVITED,
    //SERVED
    SHAVED
};

struct shmBuffer{
    int firstClientIndex;
    int lastClientIndex;
    int waitingRoomSize;
    enum barberStatus barberStatus;
    pid_t selectedClient;
    pid_t queue[MAX_QUEUE_SIZE];
} *barbershop;






#endif //IPC_SEMAPHORES_SHAREDMEMORY_COMMON_H
