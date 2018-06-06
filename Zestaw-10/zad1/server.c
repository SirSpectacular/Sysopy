#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>


#ifndef UNIX_PATH_MAX
struct sockaddr_un sizeCheck;
#define UNIX_PATH_MAX sizeof(sizeCheck.sun_path)
#endif

#define MAX_NAME_LENGTH 256
#define MAX_LOCAL_CLIENTS 20
#define MAX_REMOTE_CLIENTS 20

#define RED_COLOR "\e[1;31m"
#define RESET_COLOR "\e[0m"

#define FAILURE_EXIT(msg, ...) {                            \
    printf(RED_COLOR msg RESET_COLOR, ##__VA_ARGS__);       \
    exit(EXIT_FAILURE);                                     }
int errorCode = 0;

int unixSocketDesc;
int inetSocketDesc;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t expressionMutex = PTHREAD_MUTEX_INITIALIZER;


enum operation{
    ADD,
    SUB,
    MUL,
    DIV,
    NAME,
    RESULT,
    ERROR
};

struct {
    u_int8_t type;
    u_int16_t size;
    int arg1;
    int arg2;
} *expression;

struct message {
    u_int8_t type;
    u_int16_t size;
    void *content;
};

struct {
    int desc;
    char name[MAX_NAME_LENGTH];
    union {
        struct sockaddr_un unixAddr;
        struct sockaddr_in inetAddr;
    } addr;
}clients[MAX_LOCAL_CLIENTS + MAX_REMOTE_CLIENTS];
int clientsCounter = 0;




int main(int argc, char **argv) {
    in_port_t portID;
    char *filePath;
    pthread_t pingThreadID;
    pthread_t inputThreadID;
    int epollDesc;

    //PARSE ARGS

    if(argc != 3)
        FAILURE_EXIT("Incorrect format of comand line arguments");

    char *dump;
    portID = (in_port_t)strtol(argv[1], &dump, 10);
    if(*dump != '\0' || portID < 1024 || portID > (1 << 16))
        FAILURE_EXIT("Incorrect format of comand line arguments");

    filePath = argv[2];
    if(strlen(filePath) > UNIX_PATH_MAX);

    //INIT SOCKETS

    unixSocketDesc = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);

    const struct sockaddr_un unixAddr = {
        .sun_family = AF_UNIX
    };
    strcpy(unixAddr.sun_path, filePath);

    bind(unixSocketDesc, (const struct sockaddr *)&unixAddr, sizeof(unixAddr));
    listen(unixSocketDesc, MAX_LOCAL_CLIENTS);

    inetSocketDesc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    const struct sockaddr_in inetAddr = {
            .sin_family = AF_INET,
            .sin_port = portID,              /* port in network byte order */
            .sin_addr.s_addr = INADDR_ANY    /* address in network byte order */
    };
    bind(inetSocketDesc, (const struct sockaddr *)&inetAddr, sizeof(inetAddr));
    listen(inetSocketDesc , MAX_REMOTE_CLIENTS);

    //INIT THREADS

    pthread_attr_t pingAttr;
    pthread_attr_init(&pingAttr);
    pthread_attr_setdetachstate(&pingAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(pingThreadID, pingAttr, pingTask, );
    pthread_attr_destroy(&pingAttr);

    pthread_attr_t inputAttr;
    pthread_attr_init(&inputAttr);
    pthread_attr_setdetachstate(&inputAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(inputThreadID, inputAttr, inputTask, );
    pthread_attr_destroy(&inputAttr);

    //EPOLL

    epollDesc = epoll_create(2137);
    union epoll_data unixEpollData = {
        .fd = unixSocketDesc
    };
    struct epoll_event unixEpollEvent = {
        .events = EPOLLIN | EPOLLRDHUP,         //TODO: Sprawdzic te flagi

        .data = unixEpollData
    };
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, unixSocketDesc, &unixEpollEvent);

    union epoll_data inetEpollData = {
        .fd = inetSocketDesc
    };
    struct epoll_event inetEpollEvent = {
        .events = EPOLLIN | EPOLLRDHUP,
        .data = unixEpollData
    };
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, inetSocketDesc, &inetEpollEvent);

    //WEB TASK

    while(1){
        handleRegistery(&epollDesc);                   //TODO: wykorzystac/ zmenic rgument epoll
        handleTaskDelegation();
        handleResults(&epollDesc);
    }
}


void handleRegistery(int *epollDesc) {
    struct sockaddr_un unixTmpAddr;
    struct sockaddr_in inetTmpAddr;
    socklen_t len;


    pthread_mutex_lock(&clientsMutex);

    len = sizeof(unixTmpAddr);
    //TODO: Epoll2: unix/inet
    clients[clientsCounter].desc = accept(unixSocketDesc, (struct sockaddr *) &unixTmpAddr, &len);
    if (clients[clientsCounter].desc != -1 && len == sizeof(unixTmpAddr)) {
        clients[clientsCounter].addr.unixAddr = unixTmpAddr;
        if (!setName()) {
            //send NAME_TAKEN
            //deregister
        }
        else
            registerClient(epollDesc);
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        //TODO: throw some shit
    }

    len = sizeof(inetTmpAddr);
    clients[clientsCounter].desc = accept(inetSocketDesc, (struct sockaddr *) &inetTmpAddr, &len);
    if (clients[clientsCounter].desc != -1 && len == sizeof(inetTmpAddr)) {
        clients[clientsCounter].addr.inetAddr = inetTmpAddr;
        if (!setName())
            rejectClient();
        else
            registerClient(epollDesc);
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        //TODO: throw more shit
    }

    pthread_mutex_unlock(&clientsMutex);
}


int setName() {
    struct message msg;
    if (recv(clients[clientsCounter].desc, &msg, 3, MSG_WAITALL) == 3) { //TODO: Padding is that rly 3 bytes
        if(msg.type == NAME) {
            msg.content = malloc(msg.size);
            recv(clients[clientsCounter].desc, msg.content, msg.size, MSG_WAITALL);

            strcpy(clients[clientsCounter].name, msg.content);
            free(msg.content);
        }
        //else
            //TODO: sth went wrong
    }

    for(int i = 0; i < clientsCounter; i++)
        if(strcmp(clients[i].name, msg.content) == 0)
            return 0;


    return 1;
}

void rejectClient() {
    struct {
        u_int8_t type;
        u_int16_t size;
    } msg;
    char errorMsg[] = "Name was already taken";

    msg.type = ERROR;
    msg.size = sizeof(errorMsg);

    send(clients[clientsCounter].desc, &msg, sizeof(msg), NULL);
    send(clients[clientsCounter].desc, errorMsg, sizeof(errorMsg), NULL);
}

void registerClient(int *epollDesc) {
    union epoll_data epollData;
    struct epoll_event epollEvent;

    epollData.fd = clients[clientsCounter].desc;
    epollEvent = {
            .events = EPOLLIN | EPOLLRDHUP,         //TODO: Sprawdzic te flagi
            .data = epollData
    };
    epoll_ctl(*epollDesc, EPOLL_CTL_ADD, clients[clientsCounter].desc, &epollEvent);
    clientsCounter++;
}


void handleTaskDelegation(){
    pthread_mutex_lock(&expressionMutex);
    if(expression != NULL) {
        send(clients[rand() % clientsCounter].desc, expression, sizeof(*expression), NULL);
        free(expression);
        expression = NULL;
    } //TODO: Return code
    pthread_mutex_unlock(&expressionMutex);
}

void handleResults(int *epollDesc) {
    struct message msg;
    struct epoll_event event;
    if (epoll_wait(*epollDesc, &event, 1, 0)) {                          //TODO: 1 event == 1 msg or 1 byte?
        if (recv(event.data.fd, &msg, 3, MSG_DONTWAIT | MSG_WAITALL) == 3) {
            if (msg.type == RESULT) {
                msg.content = malloc(msg.size);
                recv(event.data.fd, msg.content, msg.size, MSG_WAITALL);
                printf("Result: %d", *(int *) msg.content);
                free(msg.content);
            }
            //else
                //sth went wrong
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            //TODO: throw more shit
        }
    }
}
