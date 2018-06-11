//
// Created by student on 07.06.18.
//
#include "common.h"

void cleanUp();
void intHandler(int sig);

int socketDesc;
struct sockaddr_un localAddr;

int main(int argc, char **argv) {
    long result;
    struct message msg;
    int connectionMode;

    atexit(cleanUp);
    signal(SIGINT, intHandler);

    union {
        struct sockaddr_un unix;
        struct sockaddr_in inet;
    } sockAddr;

    //INITIALIZE

    if (argc == 4) {
        if (strcmp(argv[2], "unix") == 0)
            connectionMode = 0;
        else FAILURE_EXIT("Incorrect format of command line arguments: mode\n")

        sockAddr.unix.sun_family = AF_UNIX;
        strcpy(sockAddr.unix.sun_path, argv[3]);


    } else if (argc == 5) {
        if (strcmp(argv[2], "inet") == 0)
            connectionMode = 1;
        else FAILURE_EXIT("Incorrect format of command line arguments: mode\n")

        sockAddr.inet.sin_family = AF_INET;

        result = inet_pton(AF_INET, argv[3], &sockAddr.inet.sin_addr);
        if (result == -1) FAILURE_EXIT("Incorrect format of command line arguments: IP\n")

        char *dump;
        in_port_t portID = (in_port_t) strtol(argv[4], &dump, 10);
        if (*dump != '\0') FAILURE_EXIT("Incorrect format of command line arguments: port\n");
        sockAddr.inet.sin_port = htons(portID);

    } else FAILURE_EXIT("Incorrect format of command line arguments: quantity\n")

    //CONNECT TO SERVER

    socketDesc = socket(connectionMode ? AF_INET : AF_UNIX, SOCK_DGRAM, 0);
    if (socketDesc == -1) FAILURE_EXIT("Was unable to create socket: %s", strerror(errno))

    if (!connectionMode) {
        srand(time(NULL));
        localAddr.sun_family = AF_UNIX;
        sprintf(localAddr.sun_path, "/tmp/%s%d", argv[1], rand());
        result = (bind(socketDesc, (struct sockaddr *) &localAddr, sizeof(struct sockaddr_un)));
        if (result == -1)
            FAILURE_EXIT("Error : Could not bind local socket\n");
    }

    result = connect(
            socketDesc,
            (struct sockaddr *) &sockAddr,
            connectionMode ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_un)
    );
    if (result < 0) FAILURE_EXIT("Was unable to connect to server: %s\n", strerror(errno))

    //SEND NAME

    msg.type = REGISTERY;
    strcpy(msg.content, argv[1]);

    result = send(socketDesc, &msg, strlen(argv[1]) + 2, 0);
    if (result == -1) FAILURE_EXIT("In 'sendName' was unable to send message: %s\n", strerror(errno))

    //MAIN LOOP

    long sollution;

    while (1) {
        result = recv(socketDesc, &msg, sizeof(msg), MSG_WAITALL);
        if (result > 0) {
            switch (msg.type) {
                case ADD:
                    sollution = (int)msg.content[0] + (int)msg.content[sizeof(int)];
                    break;
                case SUB:
                    sollution = (int)msg.content[0] - (int)msg.content[sizeof(int)];
                    break;
                case MUL:
                    sollution =  (int)msg.content[0] * (int)msg.content[sizeof(int)];
                    break;
                case DIV:
                    sollution = (int)msg.content[0] / (int)msg.content[sizeof(int)];
                    break;
                case PING: {
                    u_int8_t pongMsg = PONG;
                    result = send(socketDesc, &pongMsg, sizeof(pongMsg), MSG_WAITALL);
                    if (result != sizeof(pongMsg)) FAILURE_EXIT("Was unable to send pong message to server: %s\n", strerror(errno));
                    continue;
                }
                case ERROR: FAILURE_EXIT("Received error message from server: %s\n", (char *) msg.content)
                default: FAILURE_EXIT("Unknown type of message received\n")
            }
            struct __attribute__((__packed__)) {
                u_int8_t type;
                long sollution;
                int counter;
            } resultMsg;

            resultMsg.type = RESULT;
            resultMsg.sollution = sollution;
            resultMsg.counter = (int)msg.content[2 * sizeof(int)];

            result = send(socketDesc, &resultMsg, sizeof(resultMsg), MSG_WAITALL);
            if (result == -1)
                FAILURE_EXIT("Was unable to send result message to server: %s\n", strerror(errno));
        }
        if (result == 0)
            FAILURE_EXIT("Server has shutdown, when I tired to get message\n")
        if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            FAILURE_EXIT("Was unable to obtain results from server: %s\n", strerror(errno));

    }
}

void cleanUp(){
    close(socketDesc);
    unlink(localAddr.sun_path);

}

void intHandler(int sig){
    exit(0);
}