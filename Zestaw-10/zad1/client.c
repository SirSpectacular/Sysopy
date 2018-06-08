//
// Created by student on 07.06.18.
//
#include <memory.h>
#include "common.h"

int main(int argc, char **argv){
    long result;

    char *clientName;
    int connectionMode;
    int socketDesc;

    union{
        struct sockaddr_un unix;
        struct sockaddr_in inet;
    }sockAddr;

    //INITIALIZE

    if(argc == 4){
        if(strcmp(argv[2], "unix") == 0)
            connectionMode = 0;
        else
            FAILURE_EXIT("Incorrect format of command line arguments: mode\n")

        sockAddr.unix.sun_family = AF_UNIX;
        strcpy(sockAddr.unix.sun_path, argv[3]);
    }
    else if(argc == 5) {
        if (strcmp(argv[2], "inet") == 0)
            connectionMode = 1;
        else
            FAILURE_EXIT("Incorrect format of command line arguments: mode")

        sockAddr.inet.sin_family = AF_INET;

        result = inet_pton(AF_INET, argv[3], &sockAddr.inet.sin_addr);
        if(result == -1)
            FAILURE_EXIT("Incorrect format of command line arguments: IP\n")

        char *dump;
        in_port_t portID = (in_port_t) strtol(argv[4], &dump, 10);
        if (*dump != '\0')
            FAILURE_EXIT("Incorrect format of command line arguments: port\n");
        sockAddr.inet.sin_port = htons(portID);

    }
    else
        FAILURE_EXIT("Incorrect format of command line arguments: quantity")

    //CONNECT TO SERVER

    socketDesc = socket(connectionMode ? AF_INET : AF_UNIX, SOCK_STREAM, 0);
    if(socketDesc == -1)
        FAILURE_EXIT("Was unable to create socket: %s", strerror(errno))

    result = connect(
            socketDesc,
            (struct sockaddr*)&sockAddr,
            connectionMode ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_un)
    );
    if(result < 0)
        FAILURE_EXIT("Was unable to connect to server: %s", strerror(errno))

    //SEND NAME

    struct __attribute__((__packed__)){
        u_int8_t type;
        u_int16_t size;
    }nameMsg;
    nameMsg.type = NAME;
    nameMsg.size = (uint16_t)(strlen(clientName) + 1);
    clientName = argv[1];

    result = send(socketDesc, &nameMsg, 3, MSG_MORE);
    if(result == -1)
        FAILURE_EXIT("In 'sendName' was unable to send message header: %s\n", strerror(errno))
    result = send(socketDesc, clientName, nameMsg.size, 0);
    if(result == -1)
        FAILURE_EXIT("In 'sendName' was unable to send message: %s\n", strerror(errno))

    //MAIN LOOP

    struct message msg;
    long sollution;

    while(1){
        result = recv(socketDesc, &msg, 3, MSG_WAITALL);
        if (result == 3) {
            msg.content = malloc(msg.size);
            if(msg.size != 0)
                result = recv(socketDesc, msg.content, msg.size, MSG_WAITALL);
            if (result == msg.size || msg.size == 0) {
                switch (msg.type) {
                    case ADD:
                        sollution = ((int*)msg.content)[0] + ((int*)msg.content)[1];
                        break;
                    case SUB:
                        sollution = ((int*)msg.content)[0] - ((int*)msg.content)[1];
                        break;
                    case MUL:
                        sollution = ((int*)msg.content)[0] * ((int*)msg.content)[1];
                        break;
                    case DIV:
                        sollution = ((int*)msg.content)[0] / ((int*)msg.content)[1];
                        break;
                    case PING: {
                        struct __attribute__((__packed__)){
                            u_int8_t type;
                            u_int16_t size;
                        } pongMsg;
                        pongMsg.type = PONG;
                        pongMsg.size = 0;
                        result = send(socketDesc, &pongMsg, 3, MSG_WAITALL);
                        if(result != 3)
                            FAILURE_EXIT("Was unable to send pong message to server: %s\n", strerror(errno));
                        continue;
                    }
                    case ERROR:
                        FAILURE_EXIT("Received error message from server: %s\n", (char*)msg.content)
                    default:
                        FAILURE_EXIT("Unknown type of message received\n")
                }
                struct __attribute__((__packed__)){
                    u_int8_t type;
                    u_int16_t size;
                    long sollution;
                } resultMsg;

                resultMsg.type = RESULTS;
                resultMsg.size = sizeof(long);
                resultMsg.sollution = sollution;

                result = send(socketDesc, &resultMsg, sizeof(resultMsg), MSG_WAITALL);
                if(result == -1)
                    FAILURE_EXIT("Was unable to send result message to server: %s\n", strerror(errno));
            }
            free(msg.content);
        }
        if (result == 0)
            FAILURE_EXIT("Server has shutdown, when I tired to get message\n")
        if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
            FAILURE_EXIT("Was unable to obtain results from server: %s\n", strerror(errno));

    }
}