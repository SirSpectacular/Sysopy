

#include "common.h"

#define MAX_NAME_LENGTH 256
#define MAX_LOCAL_CLIENTS 20
#define MAX_REMOTE_CLIENTS 20

int unixSocketDesc;
int inetSocketDesc;
int epollDesc;

pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t expressionMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t expressionProcessed = PTHREAD_COND_INITIALIZER;

//------ Program structure ---------
int main(int, char**);
    void cleanUp();
    void intHandler(int);
    int cmpAddr(struct sockaddr*, struct sockaddr*);
    void unregisterClient(int);

    //communicationThread
        void handleRegistery(char*, struct sockaddr*, int);
            void rejectClient(int);
            void registerClient();
        void handleTaskDelegation();
        void handleResults();

    //pingThread
    void* pingTask(void*);


    //inputThread
    void* inputTask(void*);
        void incorrectExpression();
//-----------------------------------



struct {
    char name[MAX_NAME_LENGTH];
    int type;
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
    signal(SIGINT, intHandler);

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

    unixSocketDesc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(unixSocketDesc == -1)
        FAILURE_EXIT("Was unable to create unix socket: %s\n", strerror(errno))
    struct sockaddr_un unixAddr;
    unixAddr.sun_family = AF_UNIX;
    strcpy(unixAddr.sun_path, filePath);
    result = bind(unixSocketDesc, (const struct sockaddr *)&unixAddr, sizeof(unixAddr));
    if(result == -1)
        FAILURE_EXIT("Was unable to bind unix socket: %s\n", strerror(errno))

    inetSocketDesc = socket(AF_INET, SOCK_DGRAM, 0);
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

    union epoll_data unixEpollData = {
        .fd = unixSocketDesc
    };
    struct epoll_event unixEpollEvent = {
        .events = EPOLLIN,
        .data = unixEpollData
    };
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, unixSocketDesc, &unixEpollEvent);

    union epoll_data inetEpollData = {
        .fd = inetSocketDesc
    };
    struct epoll_event inetEpollEvent = {
        .events = EPOLLIN,
        .data = inetEpollData
    };
    epoll_ctl(epollDesc, EPOLL_CTL_ADD, inetSocketDesc, &inetEpollEvent);

    //SOCKET TASK

    while(1){
        handleTaskDelegation();
        handleResults();
    }
}

void cleanUp(){
    pthread_mutex_destroy(&clientsMutex);
    pthread_mutex_destroy(&expressionMutex);

    close(unixSocketDesc);
    unlink(filePath);

    close(inetSocketDesc);

    close(epollDesc);

    if(expression != NULL){
        free(expression);
    }
}

void intHandler(int sig){
    exit(0);
}

cmpAddr(struct sockaddr *a, struct sockaddr *b) {
    if(a->sa_family == b->sa_family && a->sa_family == AF_UNIX){
        struct sockaddr_un au = *(struct sockaddr_un*)a;
        struct sockaddr_un bu = *(struct sockaddr_un*)b;
        if(strcmp(au.sun_path, bu.sun_path) == 0)
            return 1;
    }
    else if(a->sa_family == b->sa_family && a->sa_family == AF_INET){
        struct sockaddr_in ai = *(struct sockaddr_in*)a;
        struct sockaddr_in bi = *(struct sockaddr_in*)b;
        if(ai.sin_port == bi.sin_port && ai.sin_addr.s_addr == bi.sin_addr.s_addr)
            return 1;
    }
    return 0;
}

void unregisterClient(int index){
    printf("Client unregistered: %s\n", clients[index].name);
    for(int i = index; i < clientsCounter; i++) {
        clients[i] = clients[i + 1];
    }

    clientsCounter--;
}


void handleRegistery(char *name, struct sockaddr *addr, int type) {
    pthread_mutex_lock(&clientsMutex);

    clients[clientsCounter].type = type;
    memcpy(&clients[clientsCounter].addr, addr, type ? sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in));
    strcpy(clients[clientsCounter].name, name);


    int flag = 0;
    for (int i = 0; i < clientsCounter; i++)
        if (strcmp(clients[i].name, name) == 0) {
            rejectClient(type);
            flag = 1;
        }
    if (!flag) {
        registerClient();
    }
    pthread_mutex_unlock(&clientsMutex);
}

void rejectClient(int type) {
    ssize_t result;
    struct message msg;

    msg.type = ERROR;
    strcpy(msg.content, "Name was already taken");

    result = sendto(
            clients[clientsCounter].type ? unixSocketDesc : inetSocketDesc,
            &msg,
            strlen(msg.content) + 2,
            0,
            (struct sockaddr*)&clients[clientsCounter].addr,
            type ? sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in)
    );
    if(result == -1)
        printf("In 'rejectClient' was unable to send error message: %s\n", strerror(errno));

    printf("client rejected\n");
}

void registerClient() {
    printf("client registered: %s\n", clients[clientsCounter].name);
    clientsCounter++;
}


void handleTaskDelegation() {
    ssize_t result;

    pthread_mutex_lock(&clientsMutex);
    if(clientsCounter > 0) {
        pthread_mutex_lock(&expressionMutex);
        if (expression != NULL) {
            int index = rand() % clientsCounter;
            result = sendto(
                    clients[index].type ? unixSocketDesc : inetSocketDesc,
                    expression,
                    sizeof(*expression),
                    0,
                    (struct sockaddr*)&clients[index].addr,
                    clients[index].type ? sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in)
            );
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

    union {
        struct sockaddr_un unixAddr;
        struct sockaddr_in inetAddr;
    } addr;
    socklen_t len;
    memset(&addr, 0, sizeof(addr));

    result = epoll_wait(epollDesc, &event, 1, 0);
    if (result == 1) {
        len = sizeof(addr);
        result = recvfrom(event.data.fd, &msg, BUFFER_SIZE + 1, MSG_WAITALL, (struct sockaddr*)&addr, &len);
        if (result > 0) {
            switch (msg.type) {
                case REGISTERY:
                    handleRegistery(&msg.content, &addr, event.data.fd == unixSocketDesc ? 1 : 0);
                    break;
                case RESULT:
                    printf("Result #%d: %ld\n", (int)msg.content[sizeof(long)], *(long*)msg.content);
                    break;
                case PONG:
                    pthread_mutex_lock(&clientsMutex);
                    for (int i = 0; i < clientsCounter; i++)
                        if (cmpAddr((struct sockaddr *) &addr, (struct sockaddr*)&clients[i].addr)) //ŻLE
                            clients[i].status = 1;
                    pthread_mutex_unlock(&clientsMutex);
                    break;
                case ERROR:
                    printf("Error: %s\n", (char*)msg.content);
                    break;
                default:
                    break;
            }
            return;
        }
        if (result == 0)
            printf("Client has shut down\n");
        if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            printf("Was unable to obtain results from client: %s\n", strerror(errno));

        pthread_mutex_lock(&clientsMutex);
        for (int i = 0; i < clientsCounter; i++)
            if (cmpAddr((struct sockaddr *) &addr, (struct sockaddr *) &clients[i].addr))
                unregisterClient(i);
        pthread_mutex_unlock(&clientsMutex);
    }
}


void* pingTask(void* dummy) {

    ssize_t result;
    uint8_t msg= PING;

    int clientsPinged;

    while (1) {
        pthread_mutex_lock(&clientsMutex);
        clientsPinged = clientsCounter;
        for (int i = 0; i < clientsCounter; i++) {
            clients[i].status = 0;
            result = sendto(
                    clients[i].type ? unixSocketDesc : inetSocketDesc,
                    &msg,
                    sizeof(msg),
                    0,
                    (struct sockaddr*)&clients[i].addr,
                    clients[i].type ? sizeof(struct sockaddr_un) : sizeof(struct sockaddr_in)
            );
            if (result == -1)
                printf("In 'pingTask' was unable to ping client: %s\n", strerror(errno));
        }
        pthread_mutex_unlock(&clientsMutex);

        sleep(2);

        pthread_mutex_lock(&clientsMutex);
        for (int i = 0; i < clientsPinged; i++) {
            if (clients[i].status == 0)
                unregisterClient(i);
        }
        pthread_mutex_unlock(&clientsMutex);
    }
}



void* inputTask(void* dummy) {
    char* buffer = NULL;
    size_t size = 0;
    char *dump;
    int counter = 0;

    while (1) {
        getline(&buffer, &size, stdin);
        pthread_mutex_lock(&expressionMutex);

        while(expression != NULL)
            pthread_cond_wait(&expressionProcessed, &expressionMutex);

        expression = malloc(sizeof(*expression));

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
        expression->counter = counter++;

        pthread_mutex_unlock(&expressionMutex);
    }
}

void incorrectExpression(){
    printf("Incorrect expression\n");
    free(expression);
    expression = NULL;
    pthread_mutex_unlock(&expressionMutex);
}

