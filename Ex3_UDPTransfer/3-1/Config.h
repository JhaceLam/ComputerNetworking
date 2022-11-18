#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <string>
#include <vector>
#include <io.h>
#include <fstream>
#include <sstream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

#ifndef CONFIG_H
#define CONFIG_H

// IP & Port
const u_short serverPort = 90;
const char serverIP[50] = "127.0.0.1";
const u_short clientPort = 91;
const char clientIP[50] = "127.0.0.1";

// Network parameters
const int MSS = 1 << 10;
const double RTO = 3000.0; // 3000 ms
const int MaxResendTimes = 5;
const int MaxSeqAckNum = 255;

const int MaxLengthOfFileName = 500;

const bool muteFileTranser = true;

void nextSeqAckNum(int &num);

int expectedAckNum(int lastAck);

class Segment;

class PseudoHeader {
private:
    u_long srcIP, dstIP;
    u_short reserveSpace;
    u_short length;

public:
    PseudoHeader(u_long srcIP, u_long dstIP, Segment segment);
};

class Segment {
private:
    u_short srcPort, dstPort;
    u_int seqNum, ackNum;
    u_short lengthAndFlag; // [0, 11] is for length of data, [12, 15] is for flag
    u_short checkSum;
    u_char data[MSS];

public:
    enum stateBit {SYN, ACK, FIN};
    Segment(u_short srcPort=0, u_short dstPort=0, u_int seqNum=0, u_int ackNum=0, u_short length=0);
    void setBit(stateBit s);
    bool checkStateBit(stateBit s);
    u_short getLength();
    u_short calcCheckSum(PseudoHeader *pseudoHeader);
    void setCheckSum(u_short checkSum);
    bool isCorrupt(PseudoHeader *pseudoHeader);
    static int lengthOfHeader();
    void fillData(u_char *src, int size);
    u_char *getData();
    string segmentInfo();
    u_int getSeqNum();
    u_int getAckNum();
};

enum logType {SYS, SEND, RECV, ERR};

void errorHandler(string s, bool forceToExit=true);

void logPrinter(logType type, string str);

string logGenerator(logType type, string str);

void socketModeSwitch(SOCKET socket, bool isSetBlocking);

void getFileNames(string path, vector<string> &fileNames);

class FileDescriptor {
public:
    char fileName[MaxLengthOfFileName];
    u_int fileSize;
    FileDescriptor(const char *fileName=nullptr, int sizeofFileName=0, u_int fileSize=0);
};

#endif