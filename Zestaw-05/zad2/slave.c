//
// Created by student on 14.04.18.
//

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>

int main(int argc, char **argv){
    srand(time(NULL));

    if(argc != 3) {
        perror("Wrong format of cmd line argumnts");
        exit(0);
    }

    char* dump;
    int N = (int) strtol(argv[2], &dump, 10);
    if(*dump != '\0') {
        perror("Wrong format of cmd line argumnts");
        exit(0);
    }

    FILE *pipe = fopen(argv[1], "w");
    if (pipe == NULL) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }

    pid_t myPID = getpid();

    printf("%d\n", myPID);

    for(int i = 0; i < N; i++){
    FILE *dateOutput = popen("date", "r");

    char dateBuffer[32];
    fgets(dateBuffer, sizeof(dateBuffer), dateOutput);
    fclose(dateOutput);

    char pipeBuffer[PIPE_BUF];
    sprintf(pipeBuffer, "#%d Slave with PID: %d, current date: %s\n", i + 1, myPID, dateBuffer);
    fputs(pipeBuffer, pipe);
    fflush(pipe);
    sleep(rand() % 4 + 2);
    }
    fclose(pipe);
}