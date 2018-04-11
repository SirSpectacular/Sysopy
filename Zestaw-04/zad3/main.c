//
// Created by student on 09.04.18.
//
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <wchar.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>


#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

struct {
    int receiveCounter;
    int sendCounter;
    int returnCounter;
    int childCallback;
}counters;


pid_t childPID;
pid_t parentPID;

void handleINT(int sig){
    kill(childPID, SIGKILL);
    exit(15);
}

void handleSIG2_parent(int sig, siginfo_t *info, void *ucontext) {
    counters.receiveCounter = info->si_value.sival_int;
    counters.childCallback = 1;
}

void handleSIG1_parent(int sig){
    printf("%sReceived back signal no. %s%d - %s%s from child process %s #%d\n", ANSI_YELLOW, ANSI_BLUE, sig, strsignal(sig), ANSI_YELLOW, ANSI_RESET, ++counters.returnCounter);
}

void handleSIG1_child(int sig){
    printf("%sReceived signal no. %s%d - %s%s in child process%s #%d\n", ANSI_YELLOW, ANSI_BLUE, sig, strsignal(sig), ANSI_YELLOW, ANSI_RESET, ++counters.receiveCounter);
    kill(parentPID, sig);
}

void handleSIG2_child(int sig){
    printf("%sReceived signal no. %s%d - %s%s in child process%s\n", ANSI_YELLOW, ANSI_BLUE, sig, strsignal(sig), ANSI_YELLOW, ANSI_RESET);
    union sigval returnVal;
    returnVal.sival_int = counters.receiveCounter;
    sigqueue(parentPID, sig, returnVal);
    exit(0);
}

void childTask(int sig1, int sig2){

    parentPID = getppid();

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, sig1);
    sigdelset(&set, sig2);
    sigprocmask(SIG_SETMASK, &set, NULL);

    struct sigaction actUSR1;
    actUSR1.sa_handler = handleSIG1_child;
    sigemptyset(&actUSR1.sa_mask);
    sigaddset(&actUSR1.sa_mask, sig2);
    actUSR1.sa_flags = 0;
    sigaction(sig1, &actUSR1, NULL);

    struct sigaction actUSR2;
    actUSR2.sa_handler = handleSIG2_child;
    sigemptyset(&actUSR2.sa_mask);
    actUSR2.sa_flags = 0;
    sigaction(sig2, &actUSR2, NULL);

    while(1) {pause();}
}

int parseArgs(int *Type, int *L, int argc, char **argv) {

    if(argc != 3) return 1;

    char* dump;
    *Type = (int)strtol(argv[1], &dump, 10);
    if(*dump != '\0') return 1;

    *L = (int) strtol(argv[2], &dump, 10);
    if (*dump != '\0') return 1;

    if(*Type != 1 && *Type != 2 && *Type != 3) return 1;
    if(*L < 1) return 1;
    return 0;
}


int main(int argc, char **argv){
    int Type;
    int L;
    if(parseArgs(&Type, &L, argc, argv)) {
        printf("%sWrong format of cmd line parameters%s\n", ANSI_YELLOW, ANSI_RESET);
        return 1;
    }

    counters.receiveCounter = 0;
    counters.sendCounter = 0;
    counters.returnCounter = 0;
    counters.childCallback = 0;

    int signal1;
    int signal2;

    if(Type != 3) {
        signal1 = SIGUSR1;
        signal2 = SIGUSR2;
    }
    else {
        signal1 = SIGRTMIN;
        signal2 = SIGRTMAX;
    }

    struct sigaction actINT;
    actINT.sa_handler = handleINT;
    sigemptyset(&actINT.sa_mask);
    actINT.sa_flags = 0;
    sigaction(SIGINT, &actINT, NULL);

    struct sigaction actSIG1;
    actSIG1.sa_handler = handleSIG1_parent;
    sigemptyset(&actSIG1.sa_mask);
    actSIG1.sa_flags = 0;
    sigaction(signal1, &actSIG1, NULL);


    struct sigaction actSIG2;
    actSIG2.sa_sigaction = handleSIG2_parent;
    sigemptyset(&actSIG2.sa_mask);
    actSIG2.sa_flags = SA_SIGINFO;
    sigaction(signal2, &actSIG2, NULL);

    childPID = fork();
    if(childPID == 0)
        childTask(signal1, signal2);
    sleep(3);
    for(int i = 0; i < L; i++) {
        if(!kill(childPID, signal1))
            printf("%sSent signal no. %s%d - %s%s to child process%s #%d\n", ANSI_YELLOW, ANSI_BLUE, signal1, strsignal(signal1), ANSI_YELLOW, ANSI_RESET, ++counters.sendCounter);
        else
            printf("%sFailed to send signal no. %d - %s to child process%s #%d\n", ANSI_RED, signal1, strsignal(signal1), ANSI_RESET, ++counters.sendCounter);
        if(Type == 2) while(counters.sendCounter != counters.returnCounter) {};
    }

    if(!kill(childPID, signal2))
        printf("%sSent signal no. %s%d - %s%s to child process%s\n", ANSI_YELLOW, ANSI_BLUE, signal2, strsignal(signal2), ANSI_YELLOW, ANSI_RESET);
    else
        printf("%sFailed to send signal no. %d - %s to child process%s\n", ANSI_RED, signal2, strsignal(signal2), ANSI_RESET);

    while(!counters.childCallback) {pause();}

    printf("%s\nIn Parent:\nSent %s%d (+1)%s signals.\nReceived back %s%d (+1)%s signals.%s\n",
           ANSI_YELLOW,
           ANSI_BLUE,
           counters.sendCounter,
           ANSI_YELLOW,
           counters.returnCounter == counters.sendCounter ? ANSI_BLUE : ANSI_RED,
           counters.returnCounter,
           ANSI_YELLOW,
           ANSI_RESET
    );

    printf("\n%sIn child:\nReceived %s%d (+1)%s signals.%s\n",
           ANSI_YELLOW,
           counters.receiveCounter == counters.sendCounter ? ANSI_BLUE : ANSI_RED,
           counters.receiveCounter,
           ANSI_YELLOW,
           ANSI_RESET
    );
}