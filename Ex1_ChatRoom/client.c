#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

HANDLE workEnd; // Mark the end of work

void WINAPI threadSend(LPVOID lp) {
    SOCKET clientSocket = *((SOCKET *)lp);
    char msg[BUFFER];
    char prefix[20];
    while(1) {
        fgets(msg, BUFFER, stdin);
        msg[strlen(msg) - 1] = '\0';
        strncpy(prefix, msg, 6);
        if (!strcmp(prefix, ">Send ")) {
            send(clientSocket, msg + 6, strlen(msg), 0);
            continue;
        }
        if (!strcmp(msg, ">Quit") || !strcmp(msg, ">Quit ")) {
            SetEvent(workEnd);
            break;
        }
        else {
            strcpy(msg, "Unknown command.");
        }
        WrapMessage(TYPE_ERROR, msg, (time_t)NULL);
        printf("%s\n", msg);
    }

    return;
}

void WINAPI threadRecv(LPVOID lp) {
    SOCKET clientSocket = *((SOCKET *)lp);
    char msg[FULL_LENGTH];
    int strLen = -1;
    while ( (strLen = recv(clientSocket, msg, FULL_LENGTH, 0)) != SOCKET_ERROR) {
        msg[strLen] = '\0';
        printf("%s\n", msg);
    }

    return;
}

int main(int argc, char *argv[]) {
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;
    HANDLE threads[2];
    DWORD threadIDs[2];
    int port = 90;
    u_long IPAddr = inet_addr("127.0.0.1");
    char username[NAME_LENGTH];

    // Initialize Socket DLL
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ErrorHandler(ERROR_WSA_START_UP);
    }

    // Get IP and port from input or command arguments
    char msg[FULL_LENGTH];
    char inputIP[20];
    if (argc != 3) {
        strcpy(msg, "Input the IP of server: ");
        WrapMessage(TYPE_GET, msg, (time_t)NULL);
        printf("%s", msg);
        scanf("%s", inputIP);
        IPAddr = inet_addr(inputIP);

        strcpy(msg, "Input the port of server: ");
        WrapMessage(TYPE_GET, msg, (time_t)NULL);
        printf("%s", msg);
        scanf("%d", &port);

        getchar(); // Get CR or LF
    } else {
        IPAddr = inet_addr(argv[1]);
        port = atoi(argv[2]);
    }

    // Initialzie serverAddr
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = IPAddr;
    serverAddr.sin_port = htons((u_short)port);

    // WSAStartup complete
    strcpy(msg, "WSAStartup Complete.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Initialize client socket
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        ErrorHandler(ERROR_INVALID_SOCKET);
    }
    strcpy(msg, "Socket created.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Start to connect
    if (connect(clientSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        ErrorHandler(ERROR_CONNECT);
    }
    strcpy(msg, "Connect succeeded.");
    WrapMessage(TYPE_OK, msg, (time_t)NULL);
    printf("%s\n", msg);

    // Get username
    sprintf(msg, "Input username(less than %d characters): ", NAME_LENGTH);
    WrapMessage(TYPE_GET, msg, (time_t)NULL);
    printf("%s", msg);
    fgets(username, NAME_LENGTH, stdin);
    username[strlen(username) - 1] = '\0';

    // Send username
    send(clientSocket, username, strlen(username), 0);

    // Create workStart, workEnd: manualReset, initialState = FALSE
    workEnd = CreateEvent(NULL, TRUE, FALSE, NULL);

    threads[0] = CreateThread(NULL, 0, (DWORD (*)(void *))threadSend, &clientSocket, 0, &threadIDs[0]);
    threads[1] = CreateThread(NULL, 0, (DWORD (*)(void *))threadRecv, &clientSocket, 0, &threadIDs[1]);

    WaitForSingleObject(workEnd, INFINITE);
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    strcpy(msg, "You have just left.");
    WrapMessage(TYPE_LEAVE, msg, (time_t)NULL);
    printf("%s\n", msg);
    strcpy(msg, "Disconnected.");
    WrapMessage(TYPE_INFO, msg, (time_t)NULL);
    printf("%s\n", msg);

    closesocket(clientSocket);
    WSACleanup();

    system("pause");
    return 0;
}