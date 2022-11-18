#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include "MyTimer.h"
#include "Config.h"
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

SOCKET clientSocket;
SOCKADDR_IN serverAddr, clientAddr;
int addrLen = sizeof(SOCKADDR);

int currentSeq = 0; 
int lastAck = MaxSeqAckNum;

string fileDir = ".\\src\\client";

bool connect() {
    // Send SYN to Server
    Segment segment1(clientPort, serverPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), segment1);
    segment1.setBit(Segment::SYN);
    segment1.setCheckSum(segment1.calcCheckSum(&pseudoHeader1));
    if (sendto(clientSocket, (char *)&segment1, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == -1) {
        string logStr = string("sending SYN to Server(1st step of 3-way handshake)") + segment1.segmentInfo();
        errorHandler(logStr, false);
        return false;
    } else {
        string logStr = string("Send SYN to Server(1st step of 3-way handshake)") + segment1.segmentInfo();
        logPrinter(logType::SEND, logStr);

        nextSeqAckNum(currentSeq);
    }

    // Receive SYN_ACK from Server
    socketModeSwitch(clientSocket, false); // Switch to Non-blocking
    Segment segment2;
    MyTimer timer1;
    timer1.start();
    while (true) {
        timer1.finish();
        if (timer1.getTime() > RTO) {
            errorHandler("receiving SYN_ACK from Server(2nd step of 3-way handshake)", false);
            return false;
        }
        if (recvfrom(clientSocket, (char *)&segment2, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader2(inet_addr(serverIP), inet_addr(clientIP), segment2);
            if (segment2.checkStateBit(Segment::SYN) && segment2.checkStateBit(Segment::ACK)
            && !segment2.isCorrupt(&pseudoHeader2)) {
                if (segment2.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive SYN_ACK from Server(2nd step of 3-way handshake)") + segment2.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(clientSocket, true); // Switch to blocking

    // Send ACK to Server
    Segment segment3(clientPort, serverPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader3(inet_addr(clientIP), inet_addr(serverIP), segment3);
    segment3.setBit(Segment::ACK);
    segment3.setCheckSum(segment3.calcCheckSum(&pseudoHeader3));
    if (sendto(clientSocket, (char *)&segment3, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == -1) {
        errorHandler("sending ACK to Server(3rd step of 3-way handshake)", false);
        return false;
    } else {
        string logStr = string("Send ACK to Server(3rd step of 3-way handshake)") + segment3.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    return true;
}

bool disconnect() {
    // Send FIN_ACK to Server
    Segment segment1(clientPort, serverPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), segment1);
    segment1.setBit(Segment::FIN);
    segment1.setBit(Segment::ACK);
    segment1.setCheckSum(segment1.calcCheckSum(&pseudoHeader1));
    if (sendto(clientSocket, (char *)&segment1, sizeof(Segment), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == -1) {
        errorHandler("sending FIN_ACK to Server(1st step of 4-way handshake)", false);
        return false;
    } else {
        string logStr = string("Send FIN_ACK to Server(1st step of 4-way handshake)") + segment1.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    // Receive ACK from Server
    socketModeSwitch(clientSocket, false); // Switch to Non-blocking
    Segment segment2;
    MyTimer timer1;
    timer1.start();
    while (true) {
        timer1.finish();
        if (timer1.getTime() > RTO) {
            errorHandler("receiving ACK from Server(2nd step of 4-way handshake)", false);
            return false;
        }
        if (recvfrom(clientSocket, (char *)&segment2, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader2(inet_addr(serverIP), inet_addr(clientIP), segment2);
            if (segment2.checkStateBit(Segment::ACK) && !segment2.isCorrupt(&pseudoHeader2)) {
                if (segment2.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive ACK from Server(2nd step of 4-way handshake)") + segment2.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(clientSocket, true); // Switch to blocking

    // Receive FIN_ACK from Server
    socketModeSwitch(clientSocket, false); // Switch to Non-blocking
    Segment segment3;
    MyTimer timer2;
    timer2.start();
    while (true) {
        timer2.finish();
        if (timer2.getTime() > RTO) {
            errorHandler("receiving FIN_ACK from Server(3rd step of 4-way handshake)", false);
            return false;
        }
        if (recvfrom(clientSocket, (char *)&segment3, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader3(inet_addr(serverIP), inet_addr(clientIP), segment3);
            if (segment3.checkStateBit(Segment::FIN) && segment3.checkStateBit(Segment::ACK) 
            && !segment3.isCorrupt(&pseudoHeader3)) {
                if (segment3.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive FIN_ACK from Server(3rd step of 4-way handshake)") + segment3.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(clientSocket, true); // Switch to blocking

    // Send ACK to Server
    Segment segment4(clientPort, serverPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader4(inet_addr(clientIP), inet_addr(serverIP), segment4);
    segment4.setBit(Segment::ACK);
    segment4.setCheckSum(segment4.calcCheckSum(&pseudoHeader4));
    if (sendto(clientSocket, (char *)&segment4, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == -1) {
        errorHandler("sending ACK to Server(4th step of 4-way handshake)", false);
        return false;
    } else {
        string logStr = string("Send ACK to Server(4th step of 4-way handshake)") + segment4.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    return true;
}

bool sendSegment(Segment &segment) {
    if (sendto(clientSocket, (char *)&segment, sizeof(Segment), 0, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == -1) {
        string errSrt = string("sending segment to Server") + segment.segmentInfo() + ": " + string(strerror(errno));
        errorHandler(errSrt, false);
        return false;
    } else {
        if (!muteFileTranser) {
            string logStr = string("Send segment to Server") + segment.segmentInfo();
            logPrinter(logType::SEND, logStr);
        }
    }

    Segment recvSegment;
    socketModeSwitch(clientSocket, false); // Switch to Non-blocking
    MyTimer timer;
    timer.start();
    while (true) {
        timer.finish();
        if (timer.getTime() > RTO) {
            string errSrt = string("receiving ACK from Server: Time Limit Exceeded ");
            errorHandler(errSrt, false);
            return false;
        }
        if (recvfrom(clientSocket, (char *)&recvSegment, Segment::lengthOfHeader(), 0, (SOCKADDR*)&serverAddr, &addrLen) > 0) {
            timer.finish();
            PseudoHeader pseudoHeader(inet_addr(serverIP), inet_addr(clientIP), recvSegment);
            if (recvSegment.checkStateBit(Segment::ACK) && !recvSegment.isCorrupt(&pseudoHeader)
            && recvSegment.getAckNum() == currentSeq) {
                if (!muteFileTranser) {
                    string logStr = string("Receive ACK from Server after ") + to_string(timer.getTime()) + string("ms ") + recvSegment.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                }
                nextSeqAckNum(lastAck);
                break;
            }
        }
    }
    socketModeSwitch(clientSocket, true); // Switch to blocking

    return true;
}

bool readFile(string fileName, u_char *&fileData, int &fileSize) {
    ifstream fin(fileName.c_str(), ifstream::in | ios::binary);
    if (!fin) {
        return false;
    }

    fin.seekg(0, fin.end);
    fileSize = fin.tellg();
    fileData = new u_char[fileSize];
    memset(fileData, 0, fileSize);
    fin.seekg(0, fin.beg);
    fin.read((char *)fileData, fileSize);
    fin.close();

    return true;
}

bool sendFile(string fileName) {
    int fileSize;
    u_char *fileData;

    // read file
    if (!readFile(fileName, fileData, fileSize)) {
        errorHandler(string("opening file: ") + fileName, false);
        return false;
    }

    MyTimer timer;
    timer.start();

    // send FileDescriptor
    int resendCount = 0;
    string fileNameWithoutPrefix = fileName.substr(fileDir.length() + 1, fileName.length() - fileDir.length() - 1);
    FileDescriptor fileDescriptor(fileNameWithoutPrefix.c_str(), fileNameWithoutPrefix.length(), fileSize);
    Segment segment1(clientPort, serverPort, currentSeq, lastAck, sizeof(fileDescriptor));
    segment1.setBit(Segment::ACK);
    segment1.fillData((u_char *)&fileDescriptor, sizeof(FileDescriptor));
    PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), segment1);
    segment1.setCheckSum(segment1.calcCheckSum(&pseudoHeader1));
    logPrinter(logType::SYS, "Send FileDescriptor");
    while (resendCount <= MaxResendTimes && !sendSegment(segment1)) {
        resendCount++;
        logPrinter(logType::SYS, "Resend FileDescriptor");
    }
    if (resendCount > MaxResendTimes) {
        errorHandler("sending FileDescriptor: Exceed MaxResendTimes", false);
        return false;
    }
    nextSeqAckNum(currentSeq);

    logPrinter(logType::SYS, "Start sending file...");

    // send file
    int segmentNum = (fileSize - 1) / MSS + 1;
    for (int i = 0; i < segmentNum; i++) {
        resendCount = 0;

        int dataSize = (i == segmentNum - 1) ? (fileSize - (segmentNum - 1) * MSS) : MSS;
        u_char *segData = fileData + i * MSS;
        Segment segment2(clientPort, serverPort, currentSeq, lastAck, dataSize);
        segment2.setBit(Segment::ACK);
        segment2.fillData(segData, dataSize);
        PseudoHeader pseudoHeader2(inet_addr(clientIP), inet_addr(serverIP), segment2);
        segment2.setCheckSum(segment2.calcCheckSum(&pseudoHeader2));
        while (resendCount <= MaxResendTimes && !sendSegment(segment2)) {
            resendCount++;
            logPrinter(logType::SYS, "Resend this segment");
        }
        if (resendCount > MaxResendTimes) {
            errorHandler("sending this segment: Exceed MaxResendTimes", false);
            return false;
        }
        nextSeqAckNum(currentSeq);
    }

    timer.finish();
    double averageThroughput = fileSize / timer.getTime() * MyTimer::second; // bytes/s
    char logStr[200];
    sprintf(logStr, "Finish sending file after %.3fms, with rate %.3fbyte/s", timer.getTime(), averageThroughput);
    logPrinter(logType::SYS, logStr);
    delete[] fileData;

    return true;
}

int main() {
    // WSAStartup
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        errorHandler("WSAStartup");
    } else {
        logPrinter(logType::SYS, "WSAStartup completed");
    }

    // Set serverAddr
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.S_un.S_addr = inet_addr(serverIP);

    // Set clientrAddr
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    clientAddr.sin_addr.S_un.S_addr = inet_addr(clientIP);

    // Set clientSocket
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        errorHandler("initializing clientSocket");
    } else {
        logPrinter(logType::SYS, "clientSocket initialized");
    }

    // bind
    if (bind(clientSocket, (SOCKADDR *)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
        errorHandler("binding clientSocket");
    } else {
        logPrinter(logType::SYS, "Bind clientAddr to clientSocket");
    }

    logPrinter(logType::SYS, "Try to establish connection.");
    while (!connect()) {
        errorHandler("connection establishment", false);
    }
    logPrinter(logType::SYS, "Connection established.");

    vector<string> fileNames;
    getFileNames(fileDir, fileNames);
    for (string fileName : fileNames) {
        cout << endl;
        logPrinter(logType::SYS, string("Try to send file: ") + fileName);
        if (sendFile(fileName)) {
            logPrinter(logType::SYS, string("File has been sent: ") + fileName);
        } else {
            logPrinter(logType::ERR, string("Fail to send file: ") + fileName);
        }
        // break;
    }

    cout << endl;
    logPrinter(logType::SYS, "Try to terminate connection.");
    while (!disconnect()) {
        errorHandler("connection termination");
    }
    logPrinter(logType::SYS, "Connection terminated.");

    closesocket(clientSocket);
    WSACleanup();
}