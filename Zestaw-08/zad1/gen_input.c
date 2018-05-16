//
// Created by student on 16.05.18.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv){
    //1. W, 2. H, 3. file

    int imgWidth = (int)strtol(argv[1], NULL, 10);

    int imgHeight= (int)strtol(argv[2], NULL, 10);

    FILE * file = fopen(argv[3], "w");

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
}

