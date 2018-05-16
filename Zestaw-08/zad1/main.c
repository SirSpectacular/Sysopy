#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
//TODO: errorCode
//TODO: Consider macros in parser
#define MAX_IMG_SIZE 10000
#define MAX_FILTER_SIZE 1000
#define STAT_FILE_NAME "./Times.txt"

void parseInput(FILE*);
void parseFilter(FILE*);
void saveOutput(FILE*);
void* threadsRoutine(void*);
void filterPixel(u_int8_t*, int, int);
int max(int, int);
void saveStats(struct timeval);

u_int8_t inputImgBuffer[MAX_IMG_SIZE][MAX_IMG_SIZE];
u_int8_t outputImgBuffer[MAX_IMG_SIZE][MAX_IMG_SIZE];
double filterBuffer[MAX_FILTER_SIZE][MAX_FILTER_SIZE];

int amountOfThreads;

int imgWidth = -1;
int imgHeight = -1;
int filterSize = -1;


typedef struct threadInfo{
    pthread_t threadID;
    int threadNum;
};

int main(int argc, char** argv) {
    char *inputImgFileName;
    char *filterFileName;
    char *outputImgFileName;

    if (argc != 5) {
        printf("Not enough arguments parsed to the program, should be 5\n");
        exit(EXIT_FAILURE);
    }

    char *dump;
    amountOfThreads = (int) strtol(argv[1], &dump, 10);
    if (*dump != '\0') {
        printf("Incorrect format of 1-st argument, should be unsigned integer\n");
        exit(EXIT_FAILURE);
    }


    inputImgFileName = argv[2];
    filterFileName = argv[3];
    outputImgFileName = argv[4];

    FILE *inputFile = fopen(inputImgFileName, "r");
    if (inputFile == NULL) {
        printf("%s", strerror(errno));
        exit(EXIT_FAILURE);
    }


    FILE *filterFile = fopen(filterFileName, "r");
    if (filterFile == NULL) {
        printf("%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    FILE* outputFile = fopen(outputImgFileName, "w");
    if (outputFile == NULL) {
        printf("%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    parseInput(inputFile);
    parseFilter(filterFile);

    struct threadInfo *tInfo;
    struct timeval startTime;


    tInfo = malloc(sizeof(*tInfo) * amountOfThreads);
    gettimeofday(&startTime, NULL);

    for (int i = 0; i < amountOfThreads; i++) {
        tInfo[i].threadNum = i;
        pthread_create(&tInfo[i].threadID, NULL, threadsRoutine, (void *) &tInfo[i]);
    }

    for (int i = 0; i < amountOfThreads; i++)
        pthread_join(tInfo[i].threadID, NULL);

    saveStats(startTime);
    saveOutput(outputFile);

    fclose(inputFile);
    fclose(filterFile);
    fclose(outputFile);
    free(tInfo);
}

void parseInput(FILE* file) { //TODO: header obtained by getline
    int maxPixelValue;
    fscanf(file, "P2\n %d %d\n%d\n", &imgWidth, &imgHeight, &maxPixelValue);
    if (imgWidth < 1 || imgHeight < 1 || maxPixelValue != 255) {
        printf("Wrong format of input file header");
        exit(EXIT_FAILURE);
    }

    char stringBuffer[8];
    long numBuffer;
    for (int i = 0; i < imgHeight; i++) {
        for (int j = 0; j < imgWidth; j++) {
            fscanf(file, "%s", stringBuffer);
            char *dump;
            numBuffer = strtol(stringBuffer, &dump, 10);
            if (*dump != '\0' || numBuffer < 0 || numBuffer > 255) {
                printf("Wrong format of input file, cell [%d][%d]", i + 1, j + 1);
                exit(EXIT_FAILURE);
            }
            inputImgBuffer[i][j] = (u_int8_t) numBuffer;
        }
    }
}


void parseFilter(FILE* file) { //TODO: header obtained by getline
    char stringBuffer[16];
    double numBuffer;
    char *dump;

    fscanf(file, "%s", stringBuffer);
    filterSize = (int)strtol(stringBuffer, &dump, 10);
    if (*dump != '\0' || filterSize < 1) {
        printf("Wrong format of filter file header");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < filterSize; i++) {
        for (int j = 0; j < filterSize; j++) {
            fscanf(file, "%s", stringBuffer);
            numBuffer = strtod(stringBuffer, &dump);
            if (*dump != '\0') {
                printf("Wrong format of filter file, cell [%d][%d]", i + 1, j + 1);
                exit(EXIT_FAILURE);
            }
            filterBuffer[i][j] = numBuffer;
        }
    }
}

void saveOutput(FILE* file) {
    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", imgWidth, imgHeight);
    fprintf(file, "255\n");

    for(int i = 0; i < imgHeight; i++){
        for(int j = 0; j < imgWidth; j++){
            fprintf(file, "%d ", outputImgBuffer[i][j]);
        }
        fprintf(file, "\n");
    }
}

void* threadsRoutine(void* argBuf){
    struct threadInfo *tInfo = argBuf;

    int myRow = tInfo->threadNum;
    while(myRow < imgHeight){
        for(int i = 0; i < imgWidth; i++)
            filterPixel(&outputImgBuffer[myRow][i], myRow, i);
        myRow += amountOfThreads;
    }
    return 0;
}

void filterPixel(u_int8_t *pixelValue, int x, int y) { //TODO: Does it rly work?
    double newVal = 0.0;
    for(int i = 0; i < filterSize; i++) {
        for(int j = 0; j < filterSize; j++) {
            newVal += inputImgBuffer[max(0, (x ) - (int)ceil((double)filterSize / 2) + (i ))][max(0, (y ) - (int)ceil((double)filterSize / 2) + (j ))] * filterBuffer[i][j];
        }
    }
    newVal = round(newVal);
    *pixelValue = (u_int8_t)newVal;
}

inline int max(int a, int b){
    return a > b ? a : b;
}

void saveStats(struct timeval startTime){
    struct timeval endTime;
    gettimeofday(&endTime, NULL);

    FILE *statFile = fopen(STAT_FILE_NAME, "a");
    timersub(&endTime, &startTime, &endTime);
    fprintf(statFile, "Input file size:\t\t%d x %d\nFilter size:\t\t\t%d\nAmount of threads:\t\t%d\nReal time:\t\t\t\t%ld.%06ld sec\n\n",
           imgWidth,
           imgHeight,
           filterSize,
           amountOfThreads,
           endTime.tv_sec,
           endTime.tv_usec
    );
    fclose(statFile);
}
