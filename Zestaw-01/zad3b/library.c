//
// Created by student on 14.03.18.
//

#include "library.h"

//For static allocation
char tab[MAX_ARRAY_SIZE][MAX_BLOCK_SIZE];

struct Array* allocArray(size_t arraySize, size_t blockSize, int typeOfAllocation){
    if (arraySize > 0 && blockSize > 0) {
        struct Array *array = malloc(sizeof(struct Array));
        if (array != NULL) {
            array->arraySize = arraySize;
            array->blockSize = blockSize;

            if (typeOfAllocation == 0) {
                array->array = malloc(sizeof(char *) * arraySize);
                array->typeOfAllocation = 0;
                if (array->array != NULL)
                    for(int i = 0; i < arraySize; i++)
                        array->array[i] = NULL;
                    return array;
            }
            else if (typeOfAllocation == 1 && arraySize < MAX_ARRAY_SIZE && blockSize < MAX_BLOCK_SIZE) {
                for (int i = 0; i < arraySize; i++)
                    for (int j = 0; j < blockSize; j++)
                        tab[i][j] = 0;
                array->typeOfAllocation = 1;
                return array;
            }
        }
        free(array);
    }
    return NULL;
}

int freeArray(struct Array *arrayRef) {
    if (arrayRef->typeOfAllocation == 0 && arrayRef->array != NULL) {
        for (int i = 0; i < arrayRef->arraySize; i++) {
            freeBlock(arrayRef, i);
        }
        free(arrayRef->array);
        arrayRef->array = NULL;
        return 0;
    }
    return -1;
}

int allocBlock(struct Array *arrayRef, char *string, const int index) {
    if(string != NULL && strlen(string) + 1 <= arrayRef->blockSize && index >= 0 && index < arrayRef->arraySize) {
        if (arrayRef->typeOfAllocation == 0) {
            arrayRef->array[index] = (char *) malloc(sizeof(char) * arrayRef->blockSize);
            if (arrayRef->array[index] != NULL) {
                strcpy(arrayRef->array[index], string);
                return 0;
            }
        }
        else if (arrayRef->typeOfAllocation == 1)
            strcpy(&tab[index][0], string);
            return 0;
    }
    return -1;
}

int freeBlock(struct Array *arrayRef, const int index) {
    if (index >= 0 && index < arrayRef->arraySize) {
        if (arrayRef->typeOfAllocation == 0 && arrayRef->array[index] != NULL) {
            free(arrayRef->array[index]);
            arrayRef->array[index] = NULL;
            return 0;
        }
        else if (arrayRef->typeOfAllocation == 1) {
            tab[index][0] = 0;
            return 0;
        }
    }
    return -1;
}

int findUsedBlock(struct Array *arrayRef) {
    if(arrayRef->typeOfAllocation == 0) {
        for (int i = 0; i < arrayRef->arraySize; i++) {
            if (arrayRef->array[i] != NULL) return i;
        }
    }
    else if(arrayRef->typeOfAllocation == 1) {
        for (int i = 0; i < arrayRef->arraySize; i++) {
            if (tab[i][0] != 0) return i;
        }
    }
    return -1;
}

int findUnusedBlock(struct Array *arrayRef) {
    if(arrayRef->typeOfAllocation == 0) {
        for (int i = 0; i < arrayRef->arraySize; i++) {
            if (arrayRef->array[i] == NULL) return i;
        }
    }
    else if(arrayRef->typeOfAllocation == 1) {
        for (int i = 0; i < arrayRef->arraySize; i++) {
            if (tab[i][0] == 0) return i;
        }
    }
    return -1;
}


int findMatchingBlock(struct Array *array, const int template){
    int output = -1, min = -1;
    for (int i = 0; i < array->arraySize; i++) {
        int sum = 0;
        if(array->typeOfAllocation == 0 && array->array != NULL && array->array[i] != NULL) {
            for (int j = 0; array->array[i][j] != 0 && j < array->blockSize; j++)
                sum = sum + array->array[i][j];
            if (abs(sum - template) < min || min < 0) {
                min = abs(sum - template);
                output = i;
            }
        }
        else if(array->typeOfAllocation == 1 && tab[i][0] != 0){
            for (int j = 0; j < array->blockSize && tab[i][j] != 0; j++)
                sum = sum + tab[i][j];
            if (abs(sum - template) < min || min < 0) {
                min = abs(sum - template);
                output = i;
            }
        }
    }
    return output;
}