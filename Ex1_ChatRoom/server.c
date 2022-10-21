#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

int socketCount = 0;
SOCKET sockets[MAX_THREAD_NUM];
HANDLE threads[MAX_THREAD_NUM];
DWORD threadIDs[MAX_THREAD_NUM];

HANDLE event;
// Prohibit:
// 1. One thread is getting socketCount while the other is modifing it.
// 2. Two or more threads modify socketCount at the same time.

void broadcast(char *msg, int len) {
    WaitForSingleObject(event, INFINITE);

    for (int i = 0; i < socketCount; i++) {
        send(sockets[i], msg, len, 0);
    }

    SetEvent(event);
}

void WINAPI threadFunc(LPVOID lp) {
    SOCKET clientSocket = *((SOCKET *)lp);
    int strLen = -1;
    char msg[BUFFER];
    char username[NAME_LENGTH];
    char fullMsg[FULL_LENGTH];
    char chatMsg[FULL_LENGTH];

    // Receive username
    if ( (strLen = recv(clientSocket, username, NAME_LENGTH, 0)) != SOCKET_ERROR) {
        username[strLen] = '\0';
        strcpy(fullMsg, username);
        strcat(fullMsg, " has just joined the chat room. Welcome!");
        WrapMessage(TYPE_JOIN, fullMsg, (time_t)NULL);

        printf("%s\n", fullMsg);
        broadcast(fullMsg, strlen(fullMsg));
    }

    // Receive message until client quit
    while ( (strLen = recv(clientSocket, msg, BUFFER, 0)) != SOCKET_ERROR ) {
        msg[strLen] = '\0';
        strcpy(fullMsg, username);
        strcat(fullMsg, ": ");
        strcat(fullMsg, msg);
        strcpy(chatMsg, fullMsg);
        WrapMessage(TYPE_SEND, fullMsg, (time_t)NULL);
        WrapMessage(TYPE_CHAT, chatMsg, (time_t)NULL);

        printf("%s\n", fullMsg);
        broadcast(chatMsg, strlen(fullMsg));
    }

    // Client quit - send message
    strcpy(fullMsg, username);
    strcat(fullMsg, " has just left.");
    WrapMessage(TYPE_LEAVE, fullMsg, (time_t)NULL);
    printf("%s\n", fullMsg);
    broadcast(fullMsg, strlen(fullMsg));

    // Client quit - modify socketCount and sockets
    WaitForSingleObject(event, INFINITE);
    for (int i = 0; i < socketCount; i++) {
        if (sockets[i] == clientSocket) {
            for (int j = i; j < socketCount - 1; j++) {
                sockets[j] = sockets[j + 1];
                threads[j] = threads[j + 1];
            }

            break;
        }
    }
    socketCount--;
    SetEvent(event);
    closesocket(clientSocket);
}

int main(int argc, char *argv[]) {
    SOCKET serverSocket, clientSocket;
    SOCKADDR_IN serverAddr, clientAddr;
    char hostName[100];
    int port = 90;
    struct hostent *ptrHost = NULL;
    struct in_addr hostInAddr;
    int addrSize = sizeof(SOCKADDR_IN);

    // Initialize Socket DLL
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ErrorHandler(ERROR_WSA_START_UP);
    }

    // Print Host Name
    char msg[FULL_LENGTH] = "Start server";
    WrapMessage(TYPE_INFO, msg, (time_t)NULL);
    gethostname(hostName, sizeof(hostName));
    printf("%s: %s\n", msg, hostName);

    // Get port from input or command arguments
    if (argc != 2) {
        strcpy(msg, "Input the port for the connections from clients: ");
        WrapMessage(TYPE_GET, msg, (time_t)NULL);
        printf("%s", msg);
        scanf("%d", &port);
    } else {
        port = atoi(argv[1]);
    }

    // Get IP by hostName
    ptrHost = gethostbyname(hostName);
    if (ptrHost == NULL) {
        ErrorHandler(ERROR_GETHOSTBYNAME);
    }
    int i = 0;
    for (i = 0; ptrHost->h_addr_list[i] != NULL; i++);
    memcpy(&hostInAddr, ptrHost->h_addr_list[--i], sizeof(hostInAddr));

    // Initialzie serverAddr
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr = hostInAddr;
    serverAddr.sin_port = htons(port);

    // Server initialization complete
    strcpy(msg, "WSAStartup Complete.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);
    strcpy(msg, "Server IP Address");
    WrapMessage(TYPE_INFO, msg, (time_t)NULL);
    printf("%s: %s\n", msg, inet_ntoa(hostInAddr));

    // Initialize server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        ErrorHandler(ERROR_INVALID_SOCKET);
    }
    strcpy(msg, "Socket created.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Bind serverSocket
    if (bind(serverSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        ErrorHandler(ERROR_BIND);
    }
    strcpy(msg, "Bind succeeded.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Start listen
    if (listen(serverSocket, MAX_THREAD_NUM) == SOCKET_ERROR) {
        ErrorHandler(ERROR_LISTEN);
    }
    strcpy(msg, "Waiting for connnection...");
    WrapMessage(TYPE_INFO, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Create event: autoReset, initialState = TRUE
    event = CreateEvent(NULL, FALSE, TRUE, NULL);

    // Start accept
    while (1) {
        if (socketCount >= MAX_THREAD_NUM) {
            continue;
        }

        clientSocket = accept(serverSocket, (SOCKADDR *)&clientAddr, &addrSize);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        strcpy(msg, "New connection has been built.");
        WrapMessage(TYPE_OK, msg, (time_t)NULL);
        printf("%s\n", msg);

        WaitForSingleObject(event, INFINITE);

        threads[socketCount] = CreateThread(NULL, 0, (DWORD (*)(void *))threadFunc, (LPVOID)&clientSocket, 0, &threadIDs[socketCount]);
        if (threads[socketCount] == NULL) {
            strcpy(msg, "Fail to create new thread.");
            WrapMessage(TYPE_ERROR, msg, (time_t)NULL);
            printf("%s\n", msg);
            continue;
        }
        sockets[socketCount] = clientSocket;
        socketCount++;

        SetEvent(event);
    }

    closesocket(serverSocket);
    WSACleanup();

    strcpy(msg, "Server Closed.");
    WrapMessage(TYPE_INFO, msg, (time_t)NULL);
    printf("%s\n", msg);

    return 0;
}