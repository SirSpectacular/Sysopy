//
// Created by student on 16.05.18.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <errno.h>

int main(int argc, char **argv){
    //Args: 1. W, 2. H, 3. file
    char *dump;
    int imgWidth = (int)strtol(argv[1], &dump, 10);
    if (*dump != '\0' || imgWidth < 1) {
        printf("Wrong format of cmd line arguments");
        exit(EXIT_FAILURE);
    }
    int imgHeight= (int)strtol(argv[2], &dump, 10);
    if (*dump != '\0' || imgHeight < 1) {
        printf("Wrong format of cmd line arguments");
        exit(EXIT_FAILURE);
    }
    FILE * file = fopen(argv[3], "w");
    if(file == NULL){
        printf("Was unable to open file: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", imgWidth, imgHeight);
    fprintf(file, "255\n");

    srand((uint)time(NULL));

    for(int i = 0; i < imgHeight; i++){
        for(int j = 0; j < imgWidth; j++){
            fprintf(file, "%d ", rand() % 256);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

