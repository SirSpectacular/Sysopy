//
// Created by student on 14.03.18.
//

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include "library.h"

struct Operations{
    int function;
    int param1;
    int param2;
};

void initTime(struct rusage *rusageStart, struct timeval *realtimeStart) {
    getrusage(RUSAGE_SELF, rusageStart);
    struct timezone timezone = {0, 0};
    gettimeofday(realtimeStart, &timezone);
}

void printTime(struct rusage *rusageStart, struct timeval *realtimeStart, FILE* file) {


    struct rusage *rusageEnd = malloc(sizeof(struct rusage));
    struct timeval *realtimeEnd = malloc((sizeof(struct timeval)));

    getrusage(RUSAGE_SELF, rusageEnd);
    struct timezone timezone = {0, 0};
    gettimeofday(realtimeEnd, &timezone);

    timersub(&rusageEnd->ru_stime, &rusageStart->ru_stime, &rusageEnd->ru_stime);
    timersub(&rusageEnd->ru_utime, &rusageStart->ru_utime, &rusageEnd->ru_utime);
    timersub(realtimeEnd, realtimeStart, realtimeEnd);

    printf("\tReal time:   %ld.%06ld \n", realtimeEnd->tv_sec, realtimeEnd->tv_usec);
    printf("\tUser time:   %ld.%06ld \n", rusageEnd->ru_utime.tv_sec, rusageEnd->ru_utime.tv_usec);
    printf("\tSystem time: %ld.%06ld \n", rusageEnd->ru_stime.tv_sec, rusageEnd->ru_stime.tv_usec);

    fprintf(file, "\tReal time:   %ld.%06ld \n", realtimeEnd->tv_sec, realtimeEnd->tv_usec);
    fprintf(file, "\tUser time:   %ld.%06ld \n", rusageEnd->ru_utime.tv_sec, rusageEnd->ru_utime.tv_usec);
    fprintf(file, "\tSystem time: %ld.%06ld \n", rusageEnd->ru_stime.tv_sec, rusageEnd->ru_stime.tv_usec);



    free(rusageEnd);
    free(realtimeEnd);

}

struct rusage *rusageStart;
struct timeval *realtimeStart;

char* generateString(int maxSize) {
    if (maxSize < 1) return NULL;
    char *base = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t dictLen = strlen(base);
    char *res = (char *) malloc((maxSize) * sizeof(char));

    for (int i = 0; i < maxSize-1; i++) {
        res[i] = base[rand() % dictLen];
    }
    res[maxSize-1] = '\0';

    return res;
}

void createAndFreeArrays(size_t sizeOfArray, size_t sizeOfBlock, int typeOfAllocation, int repeat, FILE *file ) {
    struct Array *array;

    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < repeat; i++) {
        if((array = allocArray(sizeOfArray, sizeOfBlock, typeOfAllocation)) == NULL) {
            printf("Something went wrong inside of createAndFreeArrays");
            break;
        }
        if(freeArray(array) == -1) {
            printf("Something went wrong inside of createAndFreeArrays");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}


void searchElement(struct Array *array, int template, int repeat, FILE *file) {

    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < repeat; i++) {
        if(findMatchingBlock(array, template) == -1) {
            printf("Something went wrong inside of searchElement");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}

void removeBlocks(struct Array *array, int amount,  FILE *file) {

    for (int i = 0; i < amount; i++){
        if (allocBlock(array, generateString((int)array->blockSize), findUnusedBlock(array))){
            printf("Something went wrong inside of addBlocks");
            break;
        }
    }
    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < amount; i++) {
        if (freeBlock(array, findUsedBlock(array))) {
            printf("Something went wrong inside of removeBlocks");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}

void addBlocks(struct Array *array, int amount, FILE *file) {

    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < amount; i++){
        if (allocBlock(array, generateString((int)array->blockSize), findUnusedBlock(array))){
            printf("Something went wrong inside of addBlocks");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}

void addAndRemoveBlocksP(struct Array *array, int amount,  FILE *file) {

    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < amount; i++) {
        if (allocBlock(array, generateString((int)array->blockSize), findUnusedBlock(array))){
            printf("Something went wrong inside of addBlocks");
            break;
        }
        if (freeBlock(array, findUsedBlock(array))) {
            printf("Something went wrong inside of removeBlocks");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}
void addAndRemoveBlocksR(struct Array *array, int amount,  FILE *file) {

    initTime(rusageStart, realtimeStart);
    for (int i = 0; i < amount; i++) {
        if (allocBlock(array, generateString((int) array->blockSize), findUnusedBlock(array))) {
            printf("Something went wrong inside of addBlocks");
            break;
        }
    }
    for (int i = 0; i < amount; i++) {
        if (freeBlock(array, findUsedBlock(array))) {
            printf("Something went wrong inside of removeBlocks");
            break;
        }
    }
    printTime(rusageStart, realtimeStart, file);
}

int parseArg(size_t *arraySize, size_t *blockSize, int *typeOfAlloc, struct Operations (*operations)[3], int argc, char **argv) {

    char *argDump;
    int current = 1;

    if (argc < 6) return 1;

    *arraySize = (size_t) strtol(argv[current], &argDump, 0);
    if (*argDump != 0) return 1;

    *blockSize = (size_t) strtol(argv[++current], &argDump, 0);
    if (*argDump != 0) return 1;


    if (strcmp(argv[++current], "dynamic") == 0 || strcmp(argv[current], "dynamic ") == 0) {
        *typeOfAlloc = 0;
    } else if (strcmp(argv[current], "static") == 0 || strcmp(argv[current], "static ") == 0) {
        *typeOfAlloc = 1;
    } else return 1;


    (*operations)[0].function = (*operations)[1].function = (*operations)[2].function = -1;

    for (int i = 0; current < argc; i++, current++) {
        if (strcmp("searchElement", argv[current])  == 0)
            (*operations)[i].function = 0;
        else if (strcmp("removeBlocks", argv[current]) == 0)
            (*operations)[i].function = 1;
        else if (strcmp("addBlocks", argv[current]) == 0)
            (*operations)[i].function = 2;
        else if (strcmp("addAndRemoveBlocksP", argv[current]) == 0)
            (*operations)[i].function = 3;
        else if (strcmp("addAndRemoveBlocksR", argv[current]) == 0)
            (*operations)[i].function = 4;
        else if (strcmp("createAndFreeArrays", argv[current]) == 0)
            (*operations)[i].function = 5;
        else return 1;

        (*operations)[i].param1 = (int) strtol(argv[++current], &argDump, 0);
        if (*argDump != 0) return 1;

        if ((*operations)[i].function == 0) {
            (*operations)[i].param2 = (int) strtol(argv[++current], &argDump, 0);
            if (*argDump != 0) return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    srand((unsigned int) time(NULL));

    size_t arraySize;
    size_t blockSize;
    int typeOfAlloc;
    struct Operations operations[3];
    parseArg(&arraySize, &blockSize, &typeOfAlloc, &operations, argc, argv);

    FILE *file;
    file = fopen("../raport2.txt", "a");
    if (!file) {
        printf ("%s \n", "Writing to file error!");
        return 1;
    }

    rusageStart = malloc(sizeof(struct rusage));
    realtimeStart = malloc((sizeof(struct timeval)));
    struct Array *array = allocArray(arraySize, blockSize, typeOfAlloc);

    for (int i = 0; i < 3; i++) {
        switch (operations[i].function) {
            case 0:
                printf("Allocated and freed  %ld arays with size %ld blocks each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Allocated and freed  %ld arays with size %ld blocks each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                searchElement(array, operations[i].param1, operations[i].param2, file);
                break;
            case 1:
                printf("Removed %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Removed %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                removeBlocks(array, operations[i].param1, file);
                break;
            case 2:
                printf("Added %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Added %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                addBlocks(array, operations[i].param1, file);
                break;
            case 3:
                printf("Added and then removed %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Added and then removed %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                addAndRemoveBlocksP(array, operations[i].param1, file);
                break;
            case 4:
                printf("Added and removed alternately %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Added and removed alternately %ld blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                addAndRemoveBlocksR(array, operations[i].param1, file);
                break;
            case 5:
                printf("Created and freed array of size %ld, made of blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                fprintf(file, "Created and freed array of size %ld, made of blocks of size %ld each, %d times. Mode %d (0-dynamic 1-static)\n", arraySize, arraySize, operations[i].param1, typeOfAlloc);
                createAndFreeArrays(arraySize, blockSize, typeOfAlloc, operations[i].param1, file);
                break;
            default:
                break;
        }
    }
    if(array->typeOfAllocation == 0) freeArray(array);
    free(rusageStart);
    free(realtimeStart);
}
