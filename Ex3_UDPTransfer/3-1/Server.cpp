#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include "MyTimer.h"
#include "Config.h"
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

SOCKET serverSocket;
SOCKADDR_IN serverAddr, clientAddr;
int addrLen = sizeof(SOCKADDR);

int currentSeq = 0; 
int lastAck = MaxSeqAckNum;

string fileDir = ".\\src\\server";

bool sendACK() {
    // Send ACK to Client
    Segment ackSegment(serverPort, clientPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeaderAck(inet_addr(serverIP), inet_addr(clientIP), ackSegment);
    ackSegment.setBit(Segment::ACK);
    ackSegment.setCheckSum(ackSegment.calcCheckSum(&pseudoHeaderAck));
    if (sendto(serverSocket, (char *)&ackSegment, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR)) == -1) {
        if (!muteFileTranser) {
            string errSrt = string("sending ACK to Client ") + ackSegment.segmentInfo() + ": " + string(strerror(errno));
            errorHandler(errSrt, false);
        }
        return false;
    } else {
        if (!muteFileTranser) {
            string logStr = string("Send ACK to Client") + ackSegment.segmentInfo();
            logPrinter(logType::SEND, logStr);
        }
    }

    return true;
}

bool connect() {
    // Receive SYN from client    
    socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
    Segment segment1;
    while (true) {
        if (recvfrom(serverSocket, (char *)&segment1, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), segment1);
            if (segment1.checkStateBit(Segment::SYN) && !segment1.isCorrupt(&pseudoHeader1)) {
                if (segment1.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive SYN from Client(1st step of 3-way handshake)") + segment1.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(serverSocket, true); // Switch to blocking
    
    // Send SYN_ACK to client
    Segment segment2(serverPort, clientPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader2(inet_addr(serverIP), inet_addr(clientIP), segment2);
    segment2.setBit(Segment::SYN);
    segment2.setBit(Segment::ACK);
    segment2.setCheckSum(segment2.calcCheckSum(&pseudoHeader2));
    if (sendto(serverSocket, (char *)&segment2, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR)) == -1) {
        string logStr = string("sending SYN_ACK to Client(2nd step of 3-way handshake)") + segment2.segmentInfo();
        errorHandler(logStr, false);
        return false;
    } else {
        string logStr = string("Send SYN_ACK to Client(2nd step of 3-way handshake)") + segment2.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    // Receive ACK from client
    socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
    Segment segment3;
    MyTimer timer2;
    timer2.start();
    while (true) {
        timer2.finish();
        if (timer2.getTime() > RTO) {
            errorHandler("receiving ACK from Client(3rd step of 3-way handshake)", false);
            return false;
        }
        if (recvfrom(serverSocket, (char *)&segment3, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader3(inet_addr(clientIP), inet_addr(serverIP), segment3);
            if (segment3.checkStateBit(Segment::ACK) && !segment3.isCorrupt(&pseudoHeader3)) {
                if (segment3.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive ACK from Client(3rd step of 3-way handshake)") + segment3.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(serverSocket, true); // Switch to blocking

    return true;
}

bool disconnect(bool ignoreFirstStep=false) {
    // Receive FIN_ACK from Client
    if (!ignoreFirstStep) {
        socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
        Segment segment1;
        MyTimer timer1;
        timer1.start();
        while (true) {
            timer1.finish();
            if (timer1.getTime() > RTO) {
                errorHandler("receiving FIN_ACK from Client(1st step of 4-way handshake)", false);
                return false;
            }
            if (recvfrom(serverSocket, (char *)&segment1, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
                PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), segment1);
                if (segment1.checkStateBit(Segment::FIN) && segment1.checkStateBit(Segment::ACK) 
                && !segment1.isCorrupt(&pseudoHeader1)) {
                    if (segment1.getSeqNum() == expectedAckNum(lastAck)) {
                        string logStr = string("Receive FIN_ACK from Client(1st step of 4-way handshake)") + segment1.segmentInfo();
                        logPrinter(logType::RECV, logStr);
                        nextSeqAckNum(lastAck);
                        break;
                    } else {
                        return false;
                    }
                }
            }
        }
        socketModeSwitch(serverSocket, true); // Switch to blocking
    }

    // Send ACK to Client
    Segment segment2(serverPort, clientPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader2(inet_addr(serverIP), inet_addr(clientIP), segment2);
    segment2.setBit(Segment::ACK);
    segment2.setCheckSum(segment2.calcCheckSum(&pseudoHeader2));
    if (sendto(serverSocket, (char *)&segment2, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR)) == -1) {
        errorHandler("sending ACK to Client(2nd step of 4-way handshake)", false);
        return false;
    } else {
        string logStr = string("Send ACK to Client(2nd step of 4-way handshake)") + segment2.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    // Send FIN_ACK to Client
    Segment segment3(serverPort, clientPort, currentSeq, lastAck, 0);
    PseudoHeader pseudoHeader3(inet_addr(serverIP), inet_addr(clientIP), segment3);
    segment3.setBit(Segment::FIN);
    segment3.setBit(Segment::ACK);
    segment3.setCheckSum(segment3.calcCheckSum(&pseudoHeader3));
    if (sendto(serverSocket, (char *)&segment3, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, sizeof(SOCKADDR)) == -1) {
        errorHandler("sending FIN_ACK to Client(3rd step of 4-way handshake)", false);
        return false;
    } else {
        string logStr = string("Send FIN_ACK to Client(3rd step of 4-way handshake)") + segment3.segmentInfo();
        logPrinter(logType::SEND, logStr);
        nextSeqAckNum(currentSeq);
    }

    // Receive ACK from Client
    socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
    Segment segment4;
    MyTimer timer2;
    timer2.start();
    while (true) {
        timer2.finish();
        if (timer2.getTime() > RTO) {
            errorHandler("receiving ACK from Client(4th step of 4-way handshake)", false);
            return false;
        }
        if (recvfrom(serverSocket, (char *)&segment4, Segment::lengthOfHeader(), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader4(inet_addr(clientIP), inet_addr(serverIP), segment4);
            if (segment4.checkStateBit(Segment::ACK) && !segment4.isCorrupt(&pseudoHeader4)) {
                if (segment4.getSeqNum() == expectedAckNum(lastAck)) {
                    string logStr = string("Receive ACK from Client(4th step of 4-way handshake)") + segment4.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    break;
                } else {
                    return false;
                }
            }
        }
    }
    socketModeSwitch(serverSocket, true); // Switch to blocking

    return true;
}

bool receiveFile() {
    // Receive FileDescriptor
    FileDescriptor fileDescriptor;
    Segment recvSegment;
    int resendCount = 0;
    MyTimer timer;
    timer.start();
    socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
    while (true) {
        timer.finish();
        if (timer.getTime() > RTO) {
            errorHandler("receiving FileDescriptor from Client: Time Limit Exceed", false);
            return true;
        }
        if (recvfrom(serverSocket, (char *)&recvSegment, sizeof(Segment), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
            PseudoHeader pseudoHeader1(inet_addr(clientIP), inet_addr(serverIP), recvSegment);
            if (!recvSegment.isCorrupt(&pseudoHeader1)) {
                // Check if is FIN_ACK
                if (recvSegment.checkStateBit(Segment::FIN) && recvSegment.checkStateBit(Segment::ACK)) {
                    cout << endl;
                    logPrinter(logType::SYS, "Try to terminate connection.");
                    string logStr = string("Receive FIN_ACK from Client(1st step of 4-way handshake)") + recvSegment.segmentInfo();
                    logPrinter(logType::RECV, logStr);
                    nextSeqAckNum(lastAck);
                    return false;
                }
                // Check if is ACK
                if (recvSegment.checkStateBit(Segment::ACK)) {
                    if (recvSegment.getSeqNum() == expectedAckNum(lastAck)) {
                        // Successfully receive FileDescriptor
                        memcpy((void *)&fileDescriptor, (void *)recvSegment.getData(), recvSegment.getLength());
                        cout << endl;
                        string logStr = string("Receive FileDescriptor from Client") + recvSegment.segmentInfo();
                        logPrinter(logType::RECV, logStr);

                        nextSeqAckNum(lastAck);
                        if (!sendACK()) return true;
                        nextSeqAckNum(currentSeq);
                        break;
                    } else {
                        // not corresponding ACK
                        socketModeSwitch(serverSocket, true); // Switch to blocking
                        if (!sendACK()) return true;
                        socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
                    }
                }
            }
        }
    }
    socketModeSwitch(serverSocket, true); // Switch to blocking

    string filePath = fileDir + string("\\") + string(fileDescriptor.fileName);
    ofstream fout(filePath.c_str(), ofstream::out | ios::binary);
    fout.seekp(0, fout.beg);
    string logStr = string("Start receving file ") + string(fileDescriptor.fileName) + string(" with size = ") + to_string(fileDescriptor.fileSize);
    logPrinter(logType::SYS, logStr);

    // Receive file
    int segmentNum = (fileDescriptor.fileSize - 1) / MSS + 1;
    for (int i = 0; i < segmentNum; i++) {
        // Receive each segment
        timer.start();
        socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
        while (true) {
            timer.finish();
            if (timer.getTime() > RTO) {
                errorHandler("receiving file segment from Client: Time Limit Exceed", false);
                return true;
            }
            if (recvfrom(serverSocket, (char *)&recvSegment, sizeof(Segment), 0, (SOCKADDR*)&clientAddr, &addrLen) > 0) {
                PseudoHeader pseudoHeader3(inet_addr(clientIP), inet_addr(serverIP), recvSegment);
                if (recvSegment.checkStateBit(Segment::ACK) && !recvSegment.isCorrupt(&pseudoHeader3)) {
                    if (recvSegment.getSeqNum() == expectedAckNum(lastAck)) {
                        // Successfully receive segment
                        fout.write((char *)recvSegment.getData(), recvSegment.getLength());
                        if (!muteFileTranser) {
                            string logStr = string("Receive file segment from Client") + recvSegment.segmentInfo();
                            logPrinter(logType::RECV, logStr);
                        }
                        
                        nextSeqAckNum(lastAck);
                        if (!sendACK()) return true;
                        nextSeqAckNum(currentSeq);
                        break;
                    } else {
                        // not corresponding ACK
                        socketModeSwitch(serverSocket, true); // Switch to blocking
                        if (!sendACK()) return true;
                        socketModeSwitch(serverSocket, false);  // Switch to Non-blocking
                    }
                }
            }
        }
        socketModeSwitch(serverSocket, true); // Switch to blocking
    }

    logStr = string("Finish receving file ") + string(fileDescriptor.fileName);
    logPrinter(logType::SYS, logStr);

    fout.close();

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

    // Set serverSocket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        errorHandler("initializing serverSocket");
    } else {
        logPrinter(logType::SYS, "serverSocket initialized");
    }

    // bind
    if (bind(serverSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        errorHandler("binding serverSocket");
    } else {
        logPrinter(logType::SYS, "Bind serverAddr to serverSocket");
    }

    logPrinter(logType::SYS, "Waiting for connnection...");
    logPrinter(logType::SYS, "Try to establish connection.");
    while (!connect()) {
        errorHandler("connection establishment", false);
    }
    logPrinter(logType::SYS, "Connection established.");

    while (receiveFile());

    if (!disconnect(true)) {
        errorHandler("connection termination", false);
        while (!disconnect()) {
            errorHandler("connection termination", false);
        }
        logPrinter(logType::SYS, "Connection terminated.");
    } else {
        logPrinter(logType::SYS, "Connection terminated.");
    }

    closesocket(serverSocket);
    WSACleanup();
}