#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")

char msgType[][10] = {"SEND", "INFO", "GET", "OK", "ERROR", "JOIN", "LEAVE", ""};

void ErrorHandler(const char *errMsg) {
    fputs(errMsg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void WrapMessage(int type, char *msg, time_t timestamp) {
    if (timestamp == (time_t)NULL) {
        timestamp = time(NULL);
    }

    struct tm *p = gmtime(&timestamp);

    char msgPrefix[PREFIX_LENGTH + BUFFER];
    if (type == TYPE_CHAT) {
        sprintf(msgPrefix, "%02d:%02d:%02d %s",
        p->tm_hour + 8,
        p->tm_min, 
        p->tm_sec, 
        msg);
    } else {
        sprintf(msgPrefix, "%02d:%02d:%02d [ %-5s ] %s",
        p->tm_hour + 8,
        p->tm_min, 
        p->tm_sec, 
        msgType[type],
        msg);
    }
    strcpy(msg, msgPrefix);

    return;
}