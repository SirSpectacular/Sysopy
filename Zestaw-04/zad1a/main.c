#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <memory.h>
#include <sys/time.h>
#include <time.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

int state;

void handleINT(int sig){
    printf("\n%sReceived signal no. %s%d - %s.%s\n",
           ANSI_YELLOW,
           ANSI_RED,
           sig,
           strsignal(sig),
           ANSI_RESET
    );
    printf("%sEnd of program%s\n", ANSI_YELLOW, ANSI_RESET);
    exit(0);
}

void handleTSTP(int sig) {
    printf("\n%sReceived signal no. %s%d - %s.%s\n",
           ANSI_YELLOW, ANSI_BLUE,
           sig,
           strsignal(sig),
           ANSI_RESET
    );
    if (state == 1) {
        printf("%sWaiting for CTRL+Z - \"continue\" or CTRL+C - \"end of program\"%s\n", ANSI_YELLOW, ANSI_RESET);
        state = 0;
    }
    else {
        printf("%sResumed program%s\n", ANSI_YELLOW, ANSI_RESET);
        state = 1;
    }
}


int main(int argc, char **argv) {
    if(argc != 1){
        printf("%sWrong format of cmd line parameters%s\n", ANSI_YELLOW, ANSI_RESET);
        return 1;
    }

    state = 1;

    struct sigaction act;
    act.sa_handler = handleTSTP;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTSTP);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    signal(SIGINT, handleINT);

    struct timeval timeBuffer;
    struct tm *nowTime;
    char tmbuf[64];

    while(1) {
        if(state){
            gettimeofday(&timeBuffer,NULL);
            nowTime = localtime(&timeBuffer.tv_sec);
            strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowTime);
            printf("%s\n", tmbuf);
            sleep(1);
        } else pause();
    }
}

