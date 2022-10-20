#ifndef CONFIG_H
#define CONFIG_H

#define PREFIX_LENGTH 30
#define NAME_LENGTH 50
#define BUFFER 1060
#define FULL_LENGTH  PREFIX_LENGTH+BUFFER
#define MAX_THREAD_NUM 6

#define TYPE_SEND 0
#define TYPE_INFO 1
#define TYPE_GET 2
#define TYPE_OK 3
#define TYPE_ERROR 4
#define TYPE_JOIN 5
#define TYPE_LEAVE 6
#define TYPE_CHAT 7

#define ERROR_WSA_START_UP "ERROR: WSAStartup() failed."
#define ERROR_INVALID_SOCKET "ERROR: Generate invalid socket."
#define ERROR_GETHOSTBYNAME "ERROR: gethostbyname() failed."
#define ERROR_BIND "ERROR: bind() failed."
#define ERROR_LISTEN "ERROR: listen() failed."
#define ERROR_CONNECT "ERROR: connect() failed."

void WrapMessage(int type, char *msg, time_t timestamp);

void ErrorHandler(const char *errMsg);

#endif