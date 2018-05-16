//
// Created by student on 16.05.18.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int doubleComparator(const void* p1, const void *p2){
    if(*(double*)p1 > *(double*)p2) return 1;
    if(*(double*)p1 < *(double*)p2) return -1;
    return 0;
}

int main(int argc, char **argv){
    //1. Size 2. Precision 3.FileName
    size_t filterSize = (size_t)strtol(argv[1], NULL, 10);
    int precision = (int)strtol(argv[2], NULL, 10);
    FILE * file = fopen(argv[3], "w");

    srand48(time(NULL));
    size_t X = filterSize * filterSize;
    double *numBuffer = malloc(sizeof(double) * (X + 1));
    for(int i = 1; i < X; i++)
        numBuffer[i] = drand48();

    qsort(&numBuffer[1], X - 1, sizeof(double), doubleComparator);
    numBuffer[X] = 1.0;
    numBuffer[0] = 0.0;

    fprintf(file, "%ld\n", filterSize);
    for(int i = 0; i < filterSize; i++){
        for(int j = 0; j < filterSize; j++){
            fprintf(file, "%.*lf ", precision, numBuffer[i * filterSize + j + 1] - numBuffer[i * filterSize + j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

