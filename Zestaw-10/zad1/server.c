#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>


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
int epollDesc;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t expressionMutex = PTHREAD_MUTEX_INITIALIZER;


enum operation{
    ADD,
    SUB,
    MUL,
    DIV,
    NAME,
    RESULTS,
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


//Program structure -------
int main(int, char**);
    void cleanUp();
    //socketTask
        void handleRegistery();
            int setName();
            void rejectClient();
            void registerClient();
        void handleTaskDelegation();
        void handleResults();
            void unregisterClient(int);
    void* pingTask(void*);
    void* inputTask(void*);
//-------------------------



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
    int result;
    in_port_t portID;
    char *filePath;
    pthread_t pingThreadID;
    pthread_t inputThreadID;

    atexit(cleanUp);

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
    if(unixSocketDesc == -1)
        FAILURE_EXIT("Was unable to create unix socket: %s", strerror(errno));
    struct sockaddr_un unixAddr;
    unixAddr.sun_family = AF_UNIX;
    strcpy(unixAddr.sun_path, filePath);
    result = bind(unixSocketDesc, (const struct sockaddr *)&unixAddr, sizeof(unixAddr));
    if(result == -1)
        FAILURE_EXIT("Was unable to bind unix socket: %s", strerror(errno));
    result = listen(unixSocketDesc, MAX_LOCAL_CLIENTS);
    if(result == -1)
        FAILURE_EXIT("Error occurred, when tired to open unix socket for connection requests: %s", strerror(errno));


    inetSocketDesc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(inetSocketDesc == -1)
        FAILURE_EXIT("Was unable to create inet socket: %s", strerror(errno));
    struct sockaddr_in inetAddr = {
            .sin_family = AF_INET,
            .sin_port = portID,              /* port in network byte order */
            .sin_addr.s_addr = INADDR_ANY    /* address in network byte order */
    };
    result = bind(inetSocketDesc, (const struct sockaddr *)&inetAddr, sizeof(inetAddr));
    if(result == -1)
        FAILURE_EXIT("Was unable to bind inet socket: %s", strerror(errno));
    result = listen(inetSocketDesc , MAX_REMOTE_CLIENTS);
    if(result == -1)
    FAILURE_EXIT("Error occurred, when tired to open inet socket for connection requests: %s", strerror(errno));

    //INIT THREADS

    pthread_attr_t pingAttr;
    pthread_attr_init(&pingAttr);
    pthread_attr_setdetachstate(&pingAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pingThreadID, &pingAttr, pingTask, NULL);
    pthread_attr_destroy(&pingAttr);

    pthread_attr_t inputAttr;
    pthread_attr_init(&inputAttr);
    pthread_attr_setdetachstate(&inputAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&inputThreadID, &inputAttr, inputTask, NULL);
    pthread_attr_destroy(&inputAttr);

    //EPOLL

    epollDesc = epoll_create(2137);
/*
    union epoll_data unixEpollData = {
        .fd = unixSocketDesc
    };
    struct epoll_event unixEpollEvent = {
        .events = EPOLLIN | EPOLLRDHUP,

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
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, inetSocketDesc, &inetEpollEvent);*/

    //SOCKET TASK

    while(1){
        handleRegistery();
        handleTaskDelegation();
        handleResults();
    }
}

void cleanUp(){
    pthread_mutex_destroy(&clientsMutex);
    pthread_mutex_destroy(&expressionMutex);

    for(int i = 0; i < clientsCounter; i++)
        close(clients[i].desc);
    close(epollDesc);

    shutdown(unixSocketDesc, SHUT_RDWR);
    close(unixSocketDesc);

    shutdown(inetSocketDesc, SHUT_RDWR);
    close(inetSocketDesc);

    if(expression != NULL){
        free(expression);
    }
}


void handleRegistery() {
    int result;

    struct sockaddr_un unixTmpAddr;
    struct sockaddr_in inetTmpAddr;
    socklen_t len;


    pthread_mutex_lock(&clientsMutex);

    len = sizeof(unixTmpAddr);

    clients[clientsCounter].desc = accept(unixSocketDesc, (struct sockaddr *) &unixTmpAddr, &len);
    if (clients[clientsCounter].desc != -1 && len == sizeof(unixTmpAddr)) {
        clients[clientsCounter].addr.unixAddr = unixTmpAddr;
        result = setName();
        if(result == 0)
            rejectClient();
        else if (result == 1)
            registerClient();
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        FAILURE_EXIT("Was unable to accept local client: %s", strerror(errno))
    }

    len = sizeof(inetTmpAddr);
    clients[clientsCounter].desc = accept(inetSocketDesc, (struct sockaddr *) &inetTmpAddr, &len);
    if (clients[clientsCounter].desc != -1 && len == sizeof(inetTmpAddr)) {
        clients[clientsCounter].addr.inetAddr = inetTmpAddr;
        result = setName();
        if(result == 0)
            rejectClient();
        else if (result == 1)
            registerClient();
    }
    else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        FAILURE_EXIT("Was unable to accept remote client: %s", strerror(errno))
    }

    pthread_mutex_unlock(&clientsMutex);
}


int setName() {
    ssize_t result;
    struct message msg;

    result = recv(clients[clientsCounter].desc, &msg, 3, MSG_WAITALL);
    if (result == 3 && msg.type == NAME) {
        msg.content = malloc(msg.size);
        result = recv(clients[clientsCounter].desc, msg.content, msg.size, MSG_WAITALL);
        if (result == msg.size) {
            strcpy(clients[clientsCounter].name, msg.content);
            free(msg.content);

            for (int i = 0; i < clientsCounter; i++)
                if (strcmp(clients[i].name, msg.content) == 0)
                    return 0;

            return 1;
        }
    }
    if (result == 0)
        printf("Client has shut down during registration");
    else
        printf("Was unable to obtain client's name: %s", strerror(errno));

    close(clients[clientsCounter].desc);
    return -1;
}
void rejectClient() {
    ssize_t result;
    struct {
        u_int8_t type;
        u_int16_t size;
    } msg;
    char errorMsg[] = "Name was already taken";

    msg.type = ERROR;
    msg.size = (u_int16_t)(strlen(errorMsg) + 1);

    result = send(clients[clientsCounter].desc, &msg, sizeof(msg), MSG_MORE);
    if(result == -1)
        printf("In 'rejectClient' was unable to send message header: %s", strerror(errno));
    result = send(clients[clientsCounter].desc, errorMsg, sizeof(errorMsg), 0);
    if(result == -1)
        printf("In 'rejectClient' was unable to send error message: %s", strerror(errno));

    close(clients[clientsCounter].desc);
}

void registerClient() {
    int result;
    union epoll_data epollData;
    struct epoll_event epollEvent;

    epollData.fd = clients[clientsCounter].desc;
    epollEvent.events = EPOLLIN;
    epollEvent.data = epollData;
    result = epoll_ctl(epollDesc, EPOLL_CTL_ADD, clients[clientsCounter].desc, &epollEvent);
    if(result == -1)
        FAILURE_EXIT("Was unable to add clients descriptor to epoll: %s", strerror(errno))
    clientsCounter++;
}


void handleTaskDelegation() {
    ssize_t result;

    pthread_mutex_lock(&clientsMutex);
    if(clientsCounter > 0) {
        pthread_mutex_lock(&expressionMutex);
        if (expression != NULL) {
            result = send(clients[rand() % clientsCounter].desc, expression, sizeof(*expression), 0);
            if (result == sizeof(*expression)) {
                free(expression);
                expression = NULL;
            } else
                printf("In 'handleTaskDelegation' was unable to send task to client: %s", strerror(errno));
        }
        pthread_mutex_unlock(&expressionMutex);
    }
    pthread_mutex_unlock(&clientsMutex);
}

void handleResults() {
    ssize_t result;

    struct message msg;
    struct epoll_event event;

    result = epoll_wait(epollDesc, &event, 1, 0);                  //TODO: 1 event == 1 msg or 1 byte?
    if (result == 1) {
        result = recv(event.data.fd, &msg, 3, MSG_DONTWAIT);
        if (result == 3 && msg.type == RESULTS) {
            msg.content = malloc(msg.size);

            result = recv(event.data.fd, msg.content, msg.size, MSG_WAITALL);
            if (result == msg.size)
                printf("Result: %d", *(int *) msg.content);

            free(msg.content);
            return;
        }
        if (result == 0)
            printf("Client has shut down when sending results");
        if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            printf("Was unable to obtain results from client: %s", strerror(errno));
        unregisterClient(event.data.fd);
    }
}

void unregisterClient(int clientDesc){
    int result;

    result = epoll_ctl(epollDesc, EPOLL_CTL_DEL, clientDesc, NULL);
    if(result == -1)
        FAILURE_EXIT("Was unable to remove clients descriptor from epoll: %s", strerror(errno))

    pthread_mutex_lock(&clientsMutex);

    for(int i = 0, flag = 0; i < clientsCounter; i++) {
        if(flag) clients[i - 1] = clients[i];
        else if(clients[i].desc == clientDesc) flag = 1;
    }

    pthread_mutex_unlock(&clientsMutex);
    close(clientDesc);
    clientsCounter--;
}

void* pingTask(void* dummy){
    
}
void* inputTask(void* dummy){

}