#ifndef COMMON_H
#define COMMON_H

#include <QtGlobal>

const qint16 SERVER_STATE_CHECK_INTERVAL = 1000;
const qint16 CLIENT_STATE_CHECK_INTERVAL = 1000;
const qint16 AUTO_CONNECT_INTERVAL = 3000;

typedef void* (*FINDMEMORYINFOFUNC)(int, int);//内存申请函数指针
extern FINDMEMORYINFOFUNC FindMemoryInfoFunc;

struct ReceiverInBuffer {
    int memID;
    char receiverName[64];
    char password[64];
    char ipAddress[16];
    quint16 port;
    float longitude;
    float latitude;
    float height;
    char detail[256];
    char mount[8];
};

struct gtime {
    time_t time;
    double sec;
};

struct Terminal {
    bool status;
    char name[64];
    char  ip[16];
    gtime timeStart;
    int timeInternal;
    int inforSendNumber;
    gtime timeDisconected;
};

struct UserHistoryLocationInfo {
    char userName[64];
    gtime time;
    double longtitude;
    double latitude;
    double height;
    char lonWE;
    char latNS;
};

#endif // COMMON_H

