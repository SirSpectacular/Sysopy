//
// Created by student on 14.03.18.
//

#ifndef STATICLIB_LIBRARY_H
#define STATICLIB_LIBRARY_H


#include <stdlib.h>
#include <memory.h>


//For static allocation
#define MAX_ARRAY_SIZE 1000000
#define MAX_BLOCK_SIZE 1000
extern char tab[MAX_ARRAY_SIZE][MAX_BLOCK_SIZE];

struct Array {
    char** array;
    size_t blockSize;
    size_t arraySize;
    int typeOfAllocation;
};

struct Array* allocArray(size_t arraySize, size_t blockSize, int typeOfAllocation);
int freeArray(struct Array *arrayRef);
int allocBlock(struct Array *arrayRef, char *string, const int index);
int findUsedBlock(struct Array *arrayRef);
int findUnusedBlock(struct Array *arrayRef);
int freeBlock(struct Array *arrayRef, int index);
int findMatchingBlock(struct Array *arrayRef, int template);

#endif //STATICLIB_LIBRARY_H
