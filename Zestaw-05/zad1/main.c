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

size_t getLineLength(FILE*);
int checkEOF(FILE*);
int extendArraySize(char***, size_t *);
int getArrayOfTokens(char*, char***, size_t);
int executeLine(char**);
int countProcesses(char**);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Wrong format of cmd line parameters");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
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

        size_t defaulListSize = 4;
        char** tokenList = malloc(sizeof(char*) * defaulListSize);
        if(getArrayOfTokens(lineBuffer, &tokenList, defaulListSize)) return 1;
        if(*tokenList == NULL) continue;
        if(executeLine(tokenList)) return 1;
        free(tokenList);
        free(lineBuffer);
    }
    fclose(file);
}


int checkEOF(FILE *file) {
    int output = fgetc(file) != EOF ? 1 : 0;
    fseek(file, -1, 1);
    return output;
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

//turns line to null ended array of strings
int getArrayOfTokens(char *line, char ***outputArray, size_t currentArraySize){
    int index = 0;
    ((*outputArray)[index++] = strtok(line, " \t\n"));
    while(((*outputArray)[index] = strtok(NULL, " \t\n"))){
        index++;
        if(index >= currentArraySize) {
            if(extendArraySize(outputArray, &currentArraySize)) return 1;
        }
    }
    (*outputArray)[index] = NULL;
    return 0;
}

//doubles the size of dynamically allocated array of strings
int extendArraySize(char ***arrayOfStrings, size_t *currentListSize) {
    char **reallocatedArgs;
    reallocatedArgs = realloc(*arrayOfStrings, 2 * *currentListSize * sizeof(char*));
    if(reallocatedArgs == NULL){
        printf("%s\n", strerror(errno));
        return 1;
    }
    *arrayOfStrings = reallocatedArgs;
    *currentListSize *= 2;
    return 0;
}

int executeLine(char** tokens) {

    int quantityOfProcesses;
    if ((quantityOfProcesses = countProcesses(tokens)) == -1) return 1;

    char ***processes = malloc(sizeof(char **) * quantityOfProcesses); //array of null ended arrays of strings

    char **tokenPtr = tokens; //this block basically initializes "processes"
    processes[0] = tokens;
    for (int i = 1; i < quantityOfProcesses; i++) {
        while (strcmp(*tokenPtr++, "|") != 0);
        *(tokenPtr - 1) = NULL;
        processes[i] = tokenPtr;
    }

    struct {
        pid_t *PIDs;
        char **names;
    } children; //will be useful during children's exit code extraction
    children.PIDs = malloc(sizeof(pid_t) * quantityOfProcesses);
    children.names = malloc(sizeof(char *) * quantityOfProcesses);

    int (*pipes)[2] = malloc(sizeof(int[2]) * (quantityOfProcesses - 1)); //TODO: Possibly can do it with only 2 pipes

    for (int i = 0; i < quantityOfProcesses; i++) {
        if (i < quantityOfProcesses - 1)
            pipe(pipes[i]);
        if ((children.PIDs[i] = fork()) == 0) {
            printf("\t%sExecuting program: \'%s\'%s\n\n", ANSI_YELLOW, processes[i][0], ANSI_RESET);

            if (i > 0)
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) perror("dup2 IN");

            if (i < quantityOfProcesses - 1) {
                close(pipes[i][0]);
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) perror("dup2 OUT");
            }
            execvp(processes[i][0], processes[i]);
            exit(errno);
        } else {
            children.names[i] = processes[i][0];
            if (i < quantityOfProcesses - 1)
                close(pipes[i][1]);
            if (i > 0)
                close(pipes[i - 1][0]);
        }
    }

    //print info about children
    while (wait(NULL) > 0);
    printf("\n");
    int output;
    for (int i = 0; i < quantityOfProcesses; i++) {
        waitpid(children.PIDs[i], &output, 0);
        if (WIFEXITED(output))
            printf("\t%sProgram \'%s\' finished with exit code %s%d - %s%s\n\n",
                   ANSI_YELLOW,
                   children.names[i],
                   WEXITSTATUS(output) == 0 ? ANSI_BLUE : ANSI_RED,
                   WEXITSTATUS(output),
                   strerror(WEXITSTATUS(output)),
                   ANSI_RESET
            );
    };
    free(children.names);
    free(children.PIDs);
    free(processes);
    free(pipes);
    return 0;
}

//TODO: Can do it in getArrayOfTokens function
int countProcesses(char **tokens){
    char **token = tokens;
    int output = 0;
    if(strcmp(*token, "|") == 0) return -1; // If line starts with "|"
    while(*token != NULL){
        if(strcmp(*token, "|") == 0) {
            output++;
            if(*(token + 1) != NULL && strcmp(*(token + 1), "|") == 0) return -1; //If there are two "|" in a row
        }
        token++;
    }
    if(strcmp(*(token - 1), "|") == 0) return -1; //If line ends with "|"
    return output + 1;
}

