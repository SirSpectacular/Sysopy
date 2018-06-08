

#include "common.h"

#define MAX_NAME_LENGTH 256
#define MAX_LOCAL_CLIENTS 20
#define MAX_REMOTE_CLIENTS 20

int unixSocketDesc;
int inetSocketDesc;
int registeryEpollDesc;
int taskEpollDesc;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t expressionMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t expressionProcessed = PTHREAD_COND_INITIALIZER;

//------ Program structure ---------
int main(int, char**);
    void cleanUp();

    //communicationThread
        void handleRegistery();
            int setName();
            void rejectClient();
            void registerClient();
        void handleTaskDelegation();
        void handleResults();

    //pingThread
    void* pingTask(void*);
        void unregisterClient(int);

    //inputThread
    void* inputTask(void*);
        void incorrectExpression();
//-----------------------------------



struct {
    int desc;
    char name[MAX_NAME_LENGTH];
    union {
        struct sockaddr_un unixAddr;
        struct sockaddr_in inetAddr;
    } addr;
    int status;
}clients[MAX_LOCAL_CLIENTS + MAX_REMOTE_CLIENTS];
int clientsCounter = 0;

pthread_t pingThreadID;
pthread_t inputThreadID;
char *filePath;

int main(int argc, char **argv) {
    int result;
    in_port_t portID;

    atexit(cleanUp);

    //PARSE ARGS

    if(argc != 3)
        FAILURE_EXIT("Incorrect format of comand line arguments\n")

    char *dump;
    portID = (in_port_t)strtol(argv[1], &dump, 10);
    if(*dump != '\0' || portID < 1024 || portID > (1 << 16))
        FAILURE_EXIT("Incorrect format of comand line arguments\n")
    filePath = argv[2];
    if(strlen(filePath) > UNIX_PATH_MAX);

    //INIT SOCKETS

    unixSocketDesc = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(unixSocketDesc == -1)
        FAILURE_EXIT("Was unable to create unix socket: %s\n", strerror(errno))
    struct sockaddr_un unixAddr;
    unixAddr.sun_family = AF_UNIX;
    strcpy(unixAddr.sun_path, filePath);
    result = bind(unixSocketDesc, (const struct sockaddr *)&unixAddr, sizeof(unixAddr));
    if(result == -1)
        FAILURE_EXIT("Was unable to bind unix socket: %s\n", strerror(errno))
    result = listen(unixSocketDesc, MAX_LOCAL_CLIENTS);
    if(result == -1)
        FAILURE_EXIT("Error occurred, when tired to open unix socket for connection requests: %s\n", strerror(errno));


    inetSocketDesc = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(inetSocketDesc == -1)
        FAILURE_EXIT("Was unable to create inet socket: %s", strerror(errno))
    struct sockaddr_in inetAddr = {
            .sin_family = AF_INET,
            .sin_port = htons(portID),              /* port in network byte order */
            .sin_addr.s_addr = INADDR_ANY           /* address in network byte order */
    };
    result = bind(inetSocketDesc, (const struct sockaddr *)&inetAddr, sizeof(inetAddr));
    if(result == -1)
        FAILURE_EXIT("Was unable to bind inet socket: %s", strerror(errno))
    result = listen(inetSocketDesc , MAX_REMOTE_CLIENTS);
    if(result == -1)
    FAILURE_EXIT("Error occurred, when tired to open inet socket for connection requests: %s\n", strerror(errno))

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

    taskEpollDesc = epoll_create(2137);
    registeryEpollDesc = epoll_create(2137);

    union epoll_data unixEpollData = {
        .fd = unixSocketDesc
    };
    struct epoll_event unixEpollEvent = {
        .events = EPOLLIN,
        .data = unixEpollData
    };
    epoll_ctl(registeryEpollDesc, EPOLL_CTL_ADD, unixSocketDesc, &unixEpollEvent);

    union epoll_data inetEpollData = {
        .fd = inetSocketDesc
    };
    struct epoll_event inetEpollEvent = {
        .events = EPOLLIN,
        .data = inetEpollData
    };
    epoll_ctl(registeryEpollDesc, EPOLL_CTL_ADD, inetSocketDesc, &inetEpollEvent);

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
    close(taskEpollDesc);

    shutdown(unixSocketDesc, SHUT_RDWR);
    close(unixSocketDesc);
    unlink(filePath);

    shutdown(inetSocketDesc, SHUT_RDWR);
    close(inetSocketDesc);

    if(expression != NULL){
        free(expression);
    }
}


void handleRegistery() {
    int result;
    struct epoll_event event;

    socklen_t length;

    pthread_mutex_lock(&clientsMutex);

    result = epoll_wait(registeryEpollDesc, &event, 1, 0);
    if(result == 1) {
        if(event.data.fd == unixSocketDesc)
            length = sizeof(struct sockaddr_un);
        else
            length = sizeof(struct sockaddr_in);

        clients[clientsCounter].desc = accept(event.data.fd, (struct sockaddr *) &clients[clientsCounter].addr, &length);

        if (clients[clientsCounter].desc != -1) {
            result = setName();
            if (result == 0)
                rejectClient();
            else if (result == 1)
                registerClient();
        } else if (clients[clientsCounter].desc == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            FAILURE_EXIT("Was unable to accept %s client: %s\n", event.data.fd == unixSocketDesc ? "local" : "remote", strerror(errno));
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

            for (int i = 0; i < clientsCounter; i++)
                if (strcmp(clients[i].name, msg.content) == 0) {
                    free(msg.content);
                    return 0;
                }
            free(msg.content);
            return 1;
        }
    }
    if (result == 0)
        printf("Client has shut down during registration\n");
    else
        printf("Was unable to obtain client's name: %s\n", strerror(errno));
    close(clients[clientsCounter].desc);
    return -1;
}
void rejectClient() {
    ssize_t result;
    struct __attribute__((__packed__)){
        u_int8_t type;
        u_int16_t size;
    } msg;
    char errorMsg[] = "Name was already taken";

    msg.type = ERROR;
    msg.size = (u_int16_t)(strlen(errorMsg) + 1);

    result = send(clients[clientsCounter].desc, &msg, sizeof(msg), MSG_MORE);
    if(result == -1)
        printf("In 'rejectClient' was unable to send message header: %s\n", strerror(errno));
    result = send(clients[clientsCounter].desc, errorMsg, sizeof(errorMsg), 0);
    if(result == -1)
        printf("In 'rejectClient' was unable to send error message: %s\n", strerror(errno));

    close(clients[clientsCounter].desc);
    printf("client rejected\n");
}

void registerClient() {
    int result;
    union epoll_data epollData;
    struct epoll_event epollEvent;

    epollData.fd = clients[clientsCounter].desc;
    epollEvent.events = EPOLLIN;
    epollEvent.data = epollData;
    result = epoll_ctl(taskEpollDesc, EPOLL_CTL_ADD, clients[clientsCounter].desc, &epollEvent);
    if(result == -1)
        FAILURE_EXIT("Was unable to add clients descriptor to epoll: %s\n", strerror(errno))
    printf("client registered: %s\n", clients[clientsCounter].name);
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
                printf("In 'handleTaskDelegation' was unable to send task to client: %s\n", strerror(errno));
        }
        pthread_mutex_unlock(&expressionMutex);
    }
    pthread_mutex_unlock(&clientsMutex);
}

void handleResults() {
    ssize_t result;

    struct message msg;
    struct epoll_event event;

    result = epoll_wait(taskEpollDesc, &event, 1, 0);
    if (result == 1) {
        result = recv(event.data.fd, &msg, 3, MSG_DONTWAIT);
        if (result == 3) {
            msg.content = malloc(msg.size);
            if(msg.size != 0)
                result = recv(event.data.fd, msg.content, msg.size, MSG_WAITALL);
            if (result == msg.size || msg.size == 0) {
                switch (msg.type) {
                    case RESULTS:
                        printf("Result: %ld\n", *(long *) msg.content);
                        break;
                    case PONG:
                        pthread_mutex_lock(&clientsMutex);
                        for(int i = 0; i < clientsCounter; i++)
                            if(clients[i].desc == event.data.fd) clients[i].status = 1;
                        pthread_mutex_unlock(&clientsMutex);
                        break;
                    case ERROR:
                        printf("Error: %s\n", (char*)msg.content);
                        break;
                    default:
                        break;
                }
                free(msg.content);
                return;
            }
            free(msg.content);
        }
        if (result == 0)
            printf("Client has shut down when sending results\n");
        if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            printf("Was unable to obtain results from client: %s\n", strerror(errno));
        unregisterClient(event.data.fd);
    }
}

void* pingTask(void* dummy) {

    ssize_t result;
    struct __attribute__((__packed__)) {
        u_int8_t type;
        u_int16_t size;
    } pingMsg;
    pingMsg.type = PING;
    pingMsg.size = 0;

    int clientsPinged;

    while (1) {
        pthread_mutex_lock(&clientsMutex);
        clientsPinged = clientsCounter;
        for (int i = 0; i < clientsCounter; i++) {
            clients[i].status = 0;
            result = send(clients[i].desc, &pingMsg, sizeof(pingMsg), MSG_WAITALL);
            if (result == -1)
                printf("In 'pingTask' was unable to ping client: %s\n", strerror(errno));
        }
        pthread_mutex_unlock(&clientsMutex);

        sleep(2);

        for (int i = 0; i < clientsPinged; i++)
            if (clients[i].status == 0)
                unregisterClient(clients[i].desc);
    }
}

void unregisterClient(int clientDesc){
    int result;

    result = epoll_ctl(taskEpollDesc, EPOLL_CTL_DEL, clientDesc, NULL);
    if(result == -1)
    FAILURE_EXIT("Was unable to remove clients descriptor from epoll: %s\n", strerror(errno))

    pthread_mutex_lock(&clientsMutex);

    for(int i = 0, flag = 0; i < clientsCounter; i++) {
        if(flag) clients[i - 1] = clients[i];
        else if(clients[i].desc == clientDesc) flag = 1;
    }

    pthread_mutex_unlock(&clientsMutex);
    printf("Client unregistered\n");
    close(clientDesc);
    clientsCounter--;
}

void* inputTask(void* dummy) {
    char* buffer = NULL;
    size_t size = 0;
    char *dump;

    while (1) {
        getline(&buffer, &size, stdin);
        pthread_mutex_lock(&expressionMutex);

        while(expression != NULL)
            pthread_cond_wait(&expressionProcessed, &expressionMutex);

        expression = malloc(sizeof(*expression));
        expression->size = sizeof(int) * 2;

        expression->arg1 = (int) strtol(strtok(buffer, " \n\t"), &dump, 10);
        if (*dump != '\0') {
            incorrectExpression();
            continue;
        }

        dump = strtok(NULL, " \n\t");
        switch (*dump) {
            case '+':
                expression->type = ADD;
                break;
            case '-':
                expression->type = SUB;
                break;
            case 'x':
            case '*':
                expression->type = MUL;
                break;
            case ':':
            case '/':
                expression->type = DIV;
                break;
            default:
                incorrectExpression();
                continue;
        }
        if(dump[1] != '\0') {
            incorrectExpression();
            continue;
        }

        expression->arg2 = (int) strtol(strtok(NULL, " \n\t"), &dump, 10);
        if (*dump != '\0') {
            incorrectExpression();
            continue;
        }

        pthread_mutex_unlock(&expressionMutex);
    }
}

void incorrectExpression(){
    printf("Incorrect expression\n");
    free(expression);
    expression = NULL;
    pthread_mutex_unlock(&expressionMutex);
}