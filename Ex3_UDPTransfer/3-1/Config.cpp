#include "Config.h"

void nextSeqAckNum(int &num) {
    num++;
    if (num > MaxSeqAckNum) num = 0;
}

int expectedAckNum(int lastAck) {
    return (lastAck + 1) % (MaxSeqAckNum + 1);
}

PseudoHeader::PseudoHeader(u_long srcIP, u_long dstIP, Segment segment) {
    this->srcIP = srcIP;
    this->dstIP = dstIP;
    this->reserveSpace = 0;
    this->length = Segment::lengthOfHeader() + segment.getLength();
}

Segment::Segment(u_short srcPort, u_short dstPort, u_int seqNum, u_int ackNum, u_short length) {
    this->srcPort = srcPort;
    this->dstPort = dstPort;
    this->seqNum = seqNum;
    this->ackNum = ackNum;
    this->lengthAndFlag = (length & 0x0FFF);
    this->checkSum = 0;
    memset(data, 0, MSS);
}

void Segment::setBit(stateBit s) {
    u_short operand = 1 << (s + 12);
    lengthAndFlag |= operand;
}

bool Segment::checkStateBit(stateBit s) {
    u_short operand = 1 << (s + 12);
    return (lengthAndFlag & operand) ? true : false;
}

u_short Segment::getLength() {
    return lengthAndFlag & 0x0FFF;
}

u_short Segment::calcCheckSum(PseudoHeader *pseudoHeader) {
    u_long sum = 0;

    int count = sizeof(PseudoHeader) / 2;
    u_short *buf = (u_short *)pseudoHeader;
    while (count--) {
        sum += *buf;
        buf++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }

    count = sizeof(Segment) / 2;
    buf = (u_short *)this;
    while (count--) {
        sum += *buf;
        buf++;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
    }

    return ~(sum & 0xFFFF);
}

void Segment::setCheckSum(u_short checkSum) {
    this->checkSum = checkSum;
}

bool Segment::isCorrupt(PseudoHeader *pseudoHeader) {
    u_short recvCheckSum = calcCheckSum(pseudoHeader);
    return recvCheckSum != 0;
}

int Segment::lengthOfHeader() {
    return sizeof(Segment) - MSS * sizeof(char);
}

void Segment::fillData(u_char *src, int size) {
    memset(data, 0, MSS);
    if (size > MSS) size = MSS;
    memcpy(data, src, size);
}

u_char *Segment::getData() {
    return data;
}

string Segment::segmentInfo() {
    char segmentInfo[200];
    bool firstFlag = true;
    ostringstream flagInfo;
    if (checkStateBit(SYN)) {
        flagInfo << "SYN";
        firstFlag = false;
    }
    if (checkStateBit(ACK)) {
        if (!firstFlag) {
            flagInfo << "_";
        }
        flagInfo << "ACK";
        firstFlag = false;
    }
    if (checkStateBit(FIN)) {
        if (!firstFlag) {
            flagInfo << "_";
        }
        flagInfo << "FIN";
        firstFlag = false;
    }

    sprintf(segmentInfo, "[seqNum = %03d, ackNum = %03d, flag=%s, lengthOfData=%d, checkSum=%04hX]", 
    seqNum, ackNum, flagInfo.str().c_str(), getLength(), checkSum);
    return string(segmentInfo);
}

u_int Segment::getSeqNum() {
    return seqNum;
}

u_int Segment::getAckNum() {
    return ackNum;
}

void errorHandler(string s, bool forceToExit) {
    string str = "Failure occured in ";
    cout << logGenerator(logType::ERR, str + s) << endl;
    if (forceToExit) {
        exit(1);
    }
}

void logPrinter(logType type, string str) {
    cout << logGenerator(type, str) << endl;
}

string logGenerator(logType type, string str) {
    string prefix = "";
    switch (type) {
        case logType::SYS:
            prefix += "[SYSTEM] ";
            break;
        case logType::SEND:
            prefix += "[SEND]   ";
            break;
        case logType::RECV:
            prefix += "[RECEIVE]";
            break;
        case logType::ERR:
        default:
            prefix += "[ERROR]  ";
            break;
    }
    return prefix + " " + str;
}

void socketModeSwitch(SOCKET socket, bool isSetBlocking) {
    u_long mode = isSetBlocking ? 0 : 1;
    ioctlsocket(socket, FIONBIO, &mode);
}

void getFileNames(string path, vector<string> &fileNames) {
    intptr_t hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
        do {
            if ((fileinfo.attrib & _A_SUBDIR)) {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0) {
                    getFileNames(p.assign(path).append("\\").append(fileinfo.name), fileNames);
                }
            } else {
                fileNames.push_back(p.assign(path).append("\\").append(fileinfo.name));
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

FileDescriptor::FileDescriptor(const char *fileName, int sizeofFileName, u_int fileSize) {
    memset(this->fileName, 0, MaxLengthOfFileName);
    if (fileName != nullptr) {
        memcpy(this->fileName, fileName, sizeofFileName);
    }
    this->fileSize = fileSize;
}