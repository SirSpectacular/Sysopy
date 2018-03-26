#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

void printTime(struct rusage *rusageStart, FILE* file) {

    struct rusage *rusageEnd = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_SELF, rusageEnd);

    timersub(&rusageEnd->ru_stime, &rusageStart->ru_stime, &rusageEnd->ru_stime);
    timersub(&rusageEnd->ru_utime, &rusageStart->ru_utime, &rusageEnd->ru_utime);

    printf("\tUser time:   %ld.%06ld \n", rusageEnd->ru_utime.tv_sec, rusageEnd->ru_utime.tv_usec);
    printf("\tSystem time: %ld.%06ld \n", rusageEnd->ru_stime.tv_sec, rusageEnd->ru_stime.tv_usec);

    fprintf(file, "\tUser time:   %ld.%06ld \n", rusageEnd->ru_utime.tv_sec, rusageEnd->ru_utime.tv_usec);
    fprintf(file, "\tSystem time: %ld.%06ld \n", rusageEnd->ru_stime.tv_sec, rusageEnd->ru_stime.tv_usec);

    free(rusageEnd);
}

void* generateRecord(size_t maxSize) {
    if (maxSize < 1) return NULL;
    void *output = (char*) malloc((maxSize) * sizeof(char));
    for (int i = 0; i < maxSize; i++)
        ((char*)output)[i] = (char)(rand() % 256);

    return output;
}

int parseArgs(int *operationToExecute, char **filename1, char **filename2, size_t *sizeOfFile, size_t *sizeOfRecord, int *libraryToUse, int argc, char **argv){
    int current = 1;
    if(strcmp(argv[current], "generate") == 0) {
        if(argc != 5) return 1;
        *operationToExecute = 0;
        *filename1 =  argv[++current];
    }
    else if(strcmp(argv[current], "sort") == 0) {
        if(argc != 6) return 1;
        *operationToExecute = 1;
        *filename1 = argv[++current];
    }
    else if(strcmp(argv[current], "copy") == 0) {
        if(argc != 7) return 1;
        *operationToExecute = 2;
        *filename1 = argv[++current];
        *filename2 = argv[++current];
    }
    else return 1;

    char *argDump;
    *sizeOfFile = (size_t) strtol(argv[++current], &argDump, 0);
    if(*argDump != 0) return 1;
    *sizeOfRecord = (size_t) strtol(argv[++current], &argDump, 0);
    if(*argDump != 0) return 1;

    if(*operationToExecute == 1 || *operationToExecute == 2){
        if(strcmp(argv[++current], "sys") == 0) {
            *libraryToUse = 0;
        }
        if(strcmp(argv[current], "lib") == 0) {
            *libraryToUse = 1;
        }
    }
    return 0;
}

int generate(char * fileName, size_t sizeOfFile, size_t sizeOfRecord) {
    FILE * file = fopen(fileName, "w");
    int errorFlag = 0;

    for(int i = 0; i < sizeOfFile; i++) {
        if(fwrite(generateRecord(sizeOfRecord), sizeOfRecord, 1, file) != 1) {
            errorFlag = 1;
            break;
        }
    }
    fclose(file);
    return errorFlag;
}

int sort(char * fileName, size_t sizeOfFile, size_t sizeOfRecord, int libraryToUse) {
    void *buffer1 = malloc(sizeOfRecord);
    void *buffer2 = malloc(sizeOfRecord);
    int errorFlag = 0;

    if(libraryToUse == 0) {
        int fileDescriptor = open(fileName, O_RDWR);
        if(fileDescriptor > 0) {
            for(int i = 0; i < sizeOfFile; i++) {
                int min = i;
                if(lseek(fileDescriptor, sizeOfRecord * i, SEEK_SET) < 0) {errorFlag = 1; break;}
                if(read(fileDescriptor, buffer1, sizeOfRecord) != sizeOfRecord) {errorFlag = 1; break;}
                for(int j = i + 1; j < sizeOfFile; j++) {
                    if(lseek(fileDescriptor, sizeOfRecord * j, SEEK_SET) < 0) {errorFlag = 1; break;}
                    if(read(fileDescriptor, buffer2, sizeOfRecord) != sizeOfRecord) {errorFlag = 1; break;}
                    if(((char*)buffer1)[0] > ((char*)buffer2)[0]) { //t[min] > t[j]
                        min = j;
                        memcpy(buffer1, buffer2, sizeOfRecord);
                    }
                }
                if(min != i) {//swap(t[i], t[min]);
                    lseek(fileDescriptor, sizeOfRecord * i, SEEK_SET);
                    read(fileDescriptor, buffer2, sizeOfRecord);
                    lseek(fileDescriptor, -sizeOfRecord, SEEK_CUR);
                    write(fileDescriptor, buffer1, sizeOfRecord);
                    lseek(fileDescriptor, sizeOfRecord * min, SEEK_SET);
                    write(fileDescriptor, buffer2, sizeOfRecord);
                }
            }
        }
        else errorFlag = 1;
        close(fileDescriptor);
    }
    else if(libraryToUse == 1) {
        FILE * file = fopen(fileName,"r+");
        if(file != NULL) {
            for(int i = 0; i < sizeOfFile; i++) {
                int min = i;
                if(fseek(file, sizeOfRecord * i, 0) != 0) {errorFlag = 1; break;}
                if(fread(buffer1, sizeOfRecord, 1, file) != 1) {errorFlag = 1; break;}
                for(int j = i + 1; j < sizeOfFile; j++) {
                    if(fseek(file, sizeOfRecord * j, 0) != 0) {errorFlag = 1; break;}
                    if(fread(buffer2, sizeOfRecord, 1, file) != 1) {errorFlag = 1; break;}
                    if(strcmp(buffer1, buffer2) > 0) { //t[min] > t[j]
                        min = j;
                        memcpy(buffer1, buffer2, sizeOfRecord);
                    }
                }
                if(min != i) {//swap(t[i], t[min]);
                    fseek(file, sizeOfRecord * i, 0);
                    fread(buffer2, sizeOfRecord, 1, file);
                    fseek(file, -sizeOfRecord, SEEK_CUR);
                    fwrite(buffer1, sizeOfRecord, 1, file);
                    fseek(file, sizeOfRecord * min, 0);
                    fwrite(buffer2, sizeOfRecord, 1, file);
                }
            }
        } else errorFlag = 1;
    }
    else errorFlag = 1;
    free(buffer1);
    free(buffer2);
    return errorFlag;
}

int copy(char *sourceFileName, char *destFileName, size_t sizeOfFile, size_t sizeOfRecord, int libraryToUse) {
    void * buffer = malloc(sizeOfRecord);
    int errorFlag = 0;

    if(libraryToUse == 0) {
        int sourceHandle = open(sourceFileName, O_RDONLY);
        int destinationHandle = open(destFileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if(sourceHandle > 0 && destinationHandle > 0) {
            for (int i = 0; i < sizeOfFile; i++) {
                if (read(sourceHandle, buffer, sizeOfRecord) != sizeOfRecord) {
                    errorFlag = 1;
                    break;
                }
                if (write(destinationHandle, buffer, sizeOfRecord) != sizeOfRecord) {
                    errorFlag = 1;
                    break;
                }
            }
        }
        else errorFlag = 1;
        close(sourceHandle);
        close(destinationHandle);
    }
    else if(libraryToUse == 1) {
        FILE * sourceFile = fopen(sourceFileName, "r");
        FILE * destFile = fopen(destFileName, "w");
        if(sourceFile != NULL && destFile != NULL)
        for(int i = 0; i < sizeOfFile; i++) {
            if(fread(buffer, sizeOfRecord, 1, sourceFile) != 1) {
                errorFlag = 1;
                break;
            }
            if(fwrite(buffer, sizeOfRecord, 1, destFile) != 1) {
                errorFlag = 1;
                break;
            }
        }
        else errorFlag = 1;
        fclose(sourceFile);
        fclose(destFile);
    }
    else errorFlag = 1;
    free(buffer);
    return errorFlag;
}

int main(int argc, char **argv) {

    int operationToExecute;
    char *filename1;
    char *filename2;
    size_t sizeOfFile;
    size_t sizeOfRecord;
    int libraryToUse;
    if(parseArgs(&operationToExecute, &filename1, &filename2, &sizeOfFile, &sizeOfRecord, &libraryToUse, argc, argv)) {
        printf("Wrong parameters");
        return 1;
    }

    FILE * outputFile = fopen("wyniki.txt", "a");
    if(!outputFile){
        printf("Was unable to open \"wyniki.txt\" file");
        return 1;
    }
    struct rusage *rusageStart = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_SELF, rusageStart);

    if(operationToExecute == 0) {
        if (generate(filename1, sizeOfFile, sizeOfRecord)) {
            printf("Error in \"generate\" method");
            return 1;
        }
        else {
            printf("Generated file with %ld records, %ld bytes each:\n", sizeOfFile, sizeOfRecord);
            fprintf(outputFile, "Generated file with %ld records, %ld bytes each:\n", sizeOfFile, sizeOfRecord);
        }
    }
    else if(operationToExecute == 1) {
        if (sort(filename1, sizeOfFile, sizeOfRecord, libraryToUse)) {
            printf("Error in \"sort\" method");
            return 1;
        }
        else {
            printf("Sorted file containing %ld records, %ld bytes each, with usage of \"%d\" functions (0-sys, 1-lib):\n", sizeOfFile, sizeOfRecord, libraryToUse);
            fprintf(outputFile, "Sorted file containing %ld records, %ld bytes each, with usage of \"%d\" functions (0-sys, 1-lib):\n", sizeOfFile, sizeOfRecord, libraryToUse);
        }
    }
    else {
        if (copy(filename1, filename2, sizeOfFile, sizeOfRecord, libraryToUse)) {
            printf("Error in \"copy\" method");
            return 1;
        }
        else {
            printf("Copied file containing %ld records, %ld bytes each to another file, with usage of \"%d\" functions (0-sys, 1-lib):\n", sizeOfFile, sizeOfRecord, libraryToUse);
            fprintf(outputFile, "Copied file containing %ld records, %ld bytes each to another file, with usage of \"%d\" functions (0-sys, 1-lib):\n", sizeOfFile, sizeOfRecord, libraryToUse);
        }
    }
    printTime(rusageStart, outputFile);
    free(rusageStart);
}

