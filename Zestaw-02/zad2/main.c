//
// Created by student on 19.03.18.
//
#define _BSD_SOURCE
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE_EXTENDED
#include <time.h>
#include <memory.h>
#include <malloc.h>
#include <dirent.h>
#include <ftw.h>
#include <stdlib.h>
#include <linux/limits.h>


int isGreater(time_t x, time_t y){
    return x > y;
}
int isSmaller(time_t x, time_t y){
    return x < y;
}
int isEqual(time_t x, time_t y){
    return x == y;
}

int parseArgs(char **dirPath, int (**cmpFuntion)(time_t, time_t), time_t *timeArg, int *mode, int argc, char **argv) {
    int current = 1;
    if(argc != 6) return 1;

    *dirPath = argv[current];

    if(strcmp(argv[++current], "<") == 0)
        *cmpFuntion = &isSmaller;
    else if(strcmp(argv[current], "=") == 0)
        *cmpFuntion = &isEqual;
    else if(strcmp(argv[current], ">") == 0)
        *cmpFuntion = &isGreater;
    else return 1;

    char *date = malloc(20);
    strcpy(date, argv[++current]);
    strcat(date, " ");
    strcat(date, argv[++current]);

    struct tm tm;
    char *dump;
    dump = strptime(date, "%Y-%m-%d %H:%M:%S", &tm);
    if(dump == NULL || *dump != '\0') return 1;
    *timeArg = mktime(&tm);

    free(date);

    if(strcmp(argv[++current], "nftw") == 0)
        *mode = 0;
    else if(strcmp(argv[current], "stat") == 0)
        *mode = 1;
    else return 1;
    return 0;
}

void getPermissions(mode_t mode, char *permissions){

    permissions = strcpy(permissions, "----------\0");

    unsigned int flags[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    char values[] = {'r', 'w', 'x'};

    for(int i = 0; i < (sizeof(flags) / sizeof(*flags)); i++)
        if(mode & flags[i]) permissions[i] = values[i % (sizeof(values) / sizeof(*values))];
}

void printFileInfo(struct stat statBuffer, char *absolutePath){
    char permissions[11];
    getPermissions(statBuffer.st_mode, permissions);
    printf("%s\t%s\t%ld\t%s", permissions, absolutePath, statBuffer.st_size,  ctime(&statBuffer.st_mtime));
}


int (*CMPFUNCTION_NFTW)(time_t, time_t);
time_t TIME_NFTW;
int fn(const char *path, const struct stat *statBuffer, int flag, struct FTW *ftwBuffer) {
    char absolutePath[PATH_MAX];
    realpath(path, absolutePath);

    if ((flag == FTW_F) && CMPFUNCTION_NFTW(statBuffer->st_mtime, TIME_NFTW))
        printFileInfo(*statBuffer, absolutePath);
    return 0;
}

void concatPath(char *path, char *filename, char *relativePath) {
    size_t pathLength = strlen(path);
    size_t filenameLength = strlen(filename);
    int slash = 0;
    if(pathLength > 0 && path[pathLength - 1] != '/') slash = 1;

    strncpy(relativePath, path, pathLength);
    if (slash == 1) strncpy(relativePath + pathLength, "/", 1);
    strncpy(relativePath + pathLength + slash, filename, filenameLength);
    relativePath[pathLength + slash + filenameLength] = '\0';
}

void statListing(char *path, int (*cmpFunction)(time_t, time_t), time_t time) {
    DIR *directory = opendir(path);
    if (directory == NULL) return;

    struct stat statBuffer;
    struct dirent *fileEntry;

    while ((fileEntry = readdir(directory)) != 0) {
        if (strcmp(fileEntry->d_name, ".") == 0 || strcmp(fileEntry->d_name, "..") == 0) continue;

        char absolutePath[PATH_MAX];
        concatPath(path, fileEntry->d_name, absolutePath);
        lstat(absolutePath, &statBuffer);

        if (S_ISDIR(statBuffer.st_mode)) {
            statListing(absolutePath, cmpFunction, time);
        }
        else if (S_ISREG(statBuffer.st_mode) && cmpFunction(statBuffer.st_mtime, time)) {
           printFileInfo(statBuffer, absolutePath);
        }
    }
    closedir(directory);
}

int main(int argc, char **argv) {

    char *dirPath;
    int mode;
    int (*cmpFunction)(time_t, time_t);
    time_t timeArg;
    if (parseArgs(&dirPath, &cmpFunction, &timeArg, &mode, argc, argv) != 0) {
        printf("Wrong parameters\n");
        return 1;
    }

    if (mode == 0) {
        CMPFUNCTION_NFTW = cmpFunction;
        TIME_NFTW = timeArg;
        if (nftw(dirPath, &fn, 100, FTW_PHYS) == -1) {
            printf("Error in nftw function\n");
            return 1;
        }
    } else {
        char absoluteDirPath[PATH_MAX];
        realpath(dirPath, absoluteDirPath);
        statListing(absoluteDirPath, cmpFunction, timeArg);
    }
}


