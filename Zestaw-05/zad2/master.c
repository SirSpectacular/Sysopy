//
// Created by student on 14.04.18.
//

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <limits.h>


int main(int argc, char **argv) {
    if(argc != 2) {
        perror("Wrong format of cmd line argumnts");
        exit(0);
    }

    if(mkfifo(argv[1], S_IRUSR	| S_IWUSR | S_IRGRP | S_IWGRP) != 0){
        printf("%s\n", strerror(errno));
        exit(errno);

    }

    FILE *pipe = fopen(argv[1], "r");
    if (pipe == NULL) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    char pipeBuffer[PIPE_BUF];
    while(fgets(pipeBuffer, PIPE_BUF, pipe)){
        printf("%s\n", pipeBuffer);
    }
    fclose(pipe);
}