#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <memory.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

int childPID;
char *getTimeProgram;
void handleINT(int sig){
    printf("\n%sReceived signal no. %s%d - %s.%s\n",
           ANSI_YELLOW,
           ANSI_RED,
           sig,
           strsignal(sig),
           ANSI_RESET
    );
    printf("%sEnd of program%s\n", ANSI_YELLOW, ANSI_RESET);
    if(!waitpid(childPID, NULL, WNOHANG))
        kill(childPID, SIGKILL);
    exit(0);
}

void handleTSTP(int sig){
    printf("\n%sReceived signal no. %s%d - %s.%s\n",
           ANSI_YELLOW, ANSI_BLUE,
           sig,
           strsignal(sig),
           ANSI_RESET
    );
    if(!waitpid(childPID, NULL, WNOHANG)) {
        kill(childPID, SIGKILL);
        printf("%sWaiting for CTRL+Z - \"continue\" or CTRL+C - \"end of program\"%s\n", ANSI_YELLOW, ANSI_RESET);
    }
    else {
        printf("%sResumed program%s\n", ANSI_YELLOW, ANSI_RESET);
        if (!(childPID = fork())) {
            execl(getTimeProgram, getTimeProgram, NULL);
        }
    }
}


int main(int argc, char **argv) {
    if(argc != 2){
        printf("%sWrong format of cmd line parameters%s\n", ANSI_YELLOW, ANSI_RESET);
        return 1;
    }

    getTimeProgram = argv[1];

    struct sigaction act;
    act.sa_handler = handleTSTP;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTSTP);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    signal(SIGINT, handleINT);

    if (!(childPID = fork())) {
        execl(getTimeProgram, getTimeProgram, NULL);
        printf("%d", childPID);
        exit(errno);
    }
    while(1) {}
}

