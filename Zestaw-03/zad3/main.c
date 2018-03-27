//
// Created by student on 23.03.18.
//

#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"


void printTime(struct rusage *rusageStart) {

    struct rusage *rusageEnd = malloc(sizeof(struct rusage));
    getrusage(RUSAGE_CHILDREN, rusageEnd);

    timersub(&rusageEnd->ru_stime, &rusageStart->ru_stime, &rusageEnd->ru_stime);
    timersub(&rusageEnd->ru_utime, &rusageStart->ru_utime, &rusageEnd->ru_utime);

    printf("\t%sUser time:   %s%ld.%06ld%s \n",
           ANSI_YELLOW,
           ANSI_BLUE,
           rusageEnd->ru_utime.tv_sec,
           rusageEnd->ru_utime.tv_usec,
           ANSI_RESET
    );
    printf("\t%sKernel time:   %s%ld.%06ld%s \n\n\n",
           ANSI_YELLOW,
           ANSI_BLUE,
           rusageEnd->ru_stime.tv_sec,
           rusageEnd->ru_stime.tv_usec,
           ANSI_RESET
    );

    free(rusageEnd);
}

size_t getLineLength(FILE *file) {
    size_t counter = 0;
    char current;
    do {
        counter++;
        current = (char)fgetc(file);
        if (current == EOF){
            counter--;
            break;
        }
    }
    while (current != '\n');
    fseek(file, -counter, 1);
    return counter;
}

int checkEOF(FILE *file) {
    int output = fgetc(file) != EOF ? 1 : 0;
    fseek(file, -1, 1);
    return output;
}

int extendList(char ***args, size_t *currentListSize) {
    char **reallocatedArgs;
    reallocatedArgs = realloc(*args, 2 * *currentListSize * sizeof(char*));
    if(reallocatedArgs == NULL){
        printf("%s\n", strerror(errno));
        return 1;
    }
    *args = reallocatedArgs;
    *currentListSize *= 2;
    return 0;
}

int getArgs(char **args, size_t currentListSize){
    int argIndex = 1;
    char *argPtr;
    while((argPtr = strtok(NULL, " \t\n"))){
        args[argIndex] = argPtr;
        argIndex++;

        if(argIndex >= currentListSize) {
            if(extendList(&args, &currentListSize)) return 1;
        }
    }
    args[argIndex] = NULL;
    return 0;
}

int executeLine(char* line, __rlim_t procLimit, __rlim_t memLimit) {
    char * program = strtok(line, " \t\n");
    int  output = 0;


    if(program) {
        size_t defaulListSize = 4;
        char** programArgs = malloc(sizeof(char*) * defaulListSize);
        programArgs[0] = program;
        if(getArgs(programArgs, defaulListSize)) return 1;

        struct rusage rusageStart;
        getrusage(RUSAGE_CHILDREN, &rusageStart);


        if (!fork()) { //execute in child's process
            struct rlimit procLimitData;
            procLimitData.rlim_max = procLimit;
            procLimitData.rlim_cur = procLimit;

            struct rlimit memLimitData;
            memLimitData.rlim_max = memLimit;
            memLimitData.rlim_cur = memLimit;

            setrlimit(RLIMIT_AS, &memLimitData);
            setrlimit(RLIMIT_CPU, &procLimitData);

            printf("\t%sExecuting program: \'%s\'%s\n\n", ANSI_YELLOW, program, ANSI_RESET);
            execvp(program, programArgs);
            exit(errno);
        }
        wait(&output);
        if(WIFSIGNALED(output))
            printf("\n\t%sProgram \'%s\' was terminated with signal %s%d - %s%s\n",
                   ANSI_YELLOW,
                   program,
                   ANSI_RED,
                   WTERMSIG(output),
                   strsignal(WTERMSIG(output)),
                   ANSI_RESET
            );
        if(WIFEXITED(output))
            printf("\n\t%sProgram \'%s\' finished with exit code %s%d - %s%s\n",
                   ANSI_YELLOW,
                   program,
                   WEXITSTATUS(output) == 0 ? ANSI_BLUE : ANSI_RED,
                   WEXITSTATUS(output),
                   strerror(WEXITSTATUS(output)),
                   ANSI_RESET
            );

        printTime(&rusageStart);
        free(programArgs);
    }
    return output;
}

int parseArgs( char **path, __rlim_t *procLimit, __rlim_t *memLimit, int argc, char **argv){
    if (argc != 4) return 1;

    *path = argv[1];

    char* dump;
    *procLimit = (__rlim_t ) strtol(argv[2], &dump, 10);
    if(*dump != '\0') return 1;
    *memLimit = (__rlim_t ) strtol(argv[3], &dump, 10);
    if(*dump != '\0') return 1;
    *memLimit *= 1000000;

    return 0;
}

int main(int argc, char **argv) {

    char* path;
    __rlim_t procLimit;
    __rlim_t memLimit;
    if(parseArgs(&path, &procLimit, &memLimit, argc, argv)) {
        printf("Wrong format of cmd line parameters");
        return 1;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    while (checkEOF(file)) {
        size_t lineLength = getLineLength(file);
        char* lineBuffer = malloc(sizeof(char) * lineLength + 1);
        if(lineBuffer == NULL) {
            printf("%s\n", strerror(errno));
            return 1;
        }
        lineBuffer[lineLength] = '\0';
        fread(lineBuffer, lineLength, 1, file);
        if(executeLine(lineBuffer, procLimit, memLimit)) return 1;
        free(lineBuffer);
    }
    fclose(file);
}


