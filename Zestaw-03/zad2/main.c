//
// Created by student on 23.03.18.
//

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

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

int executeLine(char* line) {
    char * program = strtok(line, " \t\n");
    int  output = 0;

    if(program) {
        size_t defaulListSize = 4;
        char** programArgs = malloc(sizeof(char*) * defaulListSize);
        programArgs[0] = program;
        if(getArgs(programArgs, defaulListSize)) return 1;

        if (!fork()) {
            printf("\n\t%sExecuting program: \'%s\'%s\n\n", ANSI_YELLOW, program, ANSI_RESET);
            execvp(program, programArgs);
            exit(errno);
        }
        wait(&output);

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

        free(programArgs);
    }
    return output;
}

int main(int argc, char **argv) {
    if (argc != 2) {
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
        if(executeLine(lineBuffer)) return 1;
        free(lineBuffer);
    }
    fclose(file);
}
