//
// Created by student on 08.04.18.
//

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <wait.h>

#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_RESET   "\x1b[0m"

struct{
    pid_t *pids;
    int counter;
} children;

int K;
int N;

void handleINT(int sig){
    kill(0, SIGKILL);
    exit(15);
}

void handleRuntimeSignals(int sig, siginfo_t *info, void *ucontext) {
    printf("%sReceived signal no. %s%d - %s%s, from process with PID: %s%d%s\n", ANSI_YELLOW, ANSI_BLUE, sig, strsignal(sig), ANSI_YELLOW, ANSI_BLUE, info->si_pid, ANSI_RESET);
}

void handleUSR1(int sig, siginfo_t *info, void *ucontext) {
    printf("%sReceived signal no. %s%d - %s%s, from process with PID: %s%d%s\n", ANSI_YELLOW, ANSI_BLUE, sig, strsignal(sig), ANSI_YELLOW, ANSI_BLUE, info->si_pid, ANSI_RESET);
    children.pids[children.counter] = info->si_pid;
    children.counter++;

    if(children.counter == K) {
        for(int i = 0; i < children.counter; i++) {
                kill(children.pids[i], SIGUSR2);
                printf("%sSent signal no. %s%d - %s%s to child process with PID: %s%d%s\n", ANSI_YELLOW, ANSI_BLUE,
                       SIGUSR2, strsignal(SIGUSR2), ANSI_YELLOW, ANSI_BLUE, children.pids[i], ANSI_RESET);
        }
    }
    else if (children.counter > K) {
        kill(children.pids[children.counter - 1], SIGUSR2);
        printf("%sSent signal no. %s%d - %s%s to child process with PID: %s%d%s\n", ANSI_YELLOW, ANSI_BLUE,
               SIGUSR2, strsignal(SIGUSR2), ANSI_YELLOW, ANSI_BLUE, children.pids[children.counter], ANSI_RESET);
    }


}

void handleUSR2(int sig){
    //printf("Jestem dzieckiem i otrzymałem USR2\n");
}

void childTask(){
    srand((unsigned int)getpid());

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR2);
    sigprocmask(SIG_SETMASK, &set, NULL);

    struct sigaction actUSR2;
    actUSR2.sa_handler = handleUSR2;
    sigemptyset(&actUSR2.sa_mask);
    actUSR2.sa_flags = 0;
    sigaction(SIGUSR2, &actUSR2, NULL);

    int uSecondsToSleep = rand() % 20000001;
    usleep((unsigned int)uSecondsToSleep);

    pid_t ppid = getppid();
    kill(ppid, SIGUSR1);
    pause();

    kill(ppid, SIGRTMIN + rand() % 32);

    exit(uSecondsToSleep/1000000);
}

int parseArgs(int *N, int *K, int argc, char **argv) {
    if(argc != 3) return -1;

    char* dump;
    *N = (int)strtol(argv[1], &dump, 10);
    if(*dump != '\0') return 1;
    *K = (int)strtol(argv[2], &dump, 10);
    if(*dump != '\0') return 1;

    if(*N < 0 || *K < 0) return -1;

    return 0;
}


int main(int argc, char **argv){

    if(parseArgs(&N, &K, argc, argv)){
        printf("%sWrong format of cmd line parameters%s\n", ANSI_YELLOW, ANSI_RESET);
        return 1;
    }

    struct sigaction actINT;
    actINT.sa_handler = handleINT;
    sigemptyset(&actINT.sa_mask);
    actINT.sa_flags = 0;
    sigaction(SIGINT, &actINT, NULL);

    struct sigaction actUSR1;
    actUSR1.sa_sigaction = handleUSR1;
    sigemptyset(&actUSR1.sa_mask);
    actUSR1.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &actUSR1, NULL);

    struct sigaction actRuntime;
    actRuntime.sa_sigaction = handleRuntimeSignals;
    sigemptyset(&actRuntime.sa_mask);
    actRuntime.sa_flags = SA_SIGINFO;
    for(int i = 0; i < 32; i++){
        sigaction(SIGRTMIN + i, &actRuntime, NULL);
    }

    children.pids = malloc(N * sizeof(pid_t));
    children.counter = 0;

    pid_t childPID;
    for(int i = 0; i < N; i++)
        if(!(childPID = fork()))
            childTask();
        else
            printf("%sSpawned child process with PID: %s%d%s\n", ANSI_YELLOW, ANSI_BLUE, childPID, ANSI_RESET);

    while(children.counter != N) {}

    int status;
    for(int i = 0; i < N; i++){
        do {
            int w = waitpid(children.pids[i], &status, WUNTRACED | WCONTINUED);
            if (w == -1) {
                perror("waitpid");
            }

            if (WIFEXITED(status)) {
                printf("%sChild with PID: %s%d%s ended with status code %s%d%s\n", ANSI_YELLOW, ANSI_BLUE, children.pids[i], ANSI_YELLOW, ANSI_BLUE, WEXITSTATUS(status), ANSI_RESET);
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("continued\n");
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    exit(EXIT_SUCCESS);
}