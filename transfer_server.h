#ifndef TRANSFERSERVER_H
#define TRANSFERSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QPair>
#include <QMutex>
#include <QTextStream>

#include "shared_buffer.h"
#include "rtklib.h"

class TransferServer : public QObject
{
    Q_OBJECT

public:
    explicit TransferServer(QObject* parent = 0);
    void tranferBuffer(const char* start, int length, int memID);

private slots:
    void incomingConnection();
    void handleReceivedData();
    void handleError(QAbstractSocket::SocketError error);
    void handleDisconnect();

    void handle500MSTimer();
    void handle1STimer();

private:
    QTcpServer* server;
    QTcpSocket* socket;
    QMutex socketWriteMutex;
    QByteArray startFlag;
    QByteArray endFlag;
    QByteArray data;
    bool flag_1, flag_2, flag_3;
    SharedBuffer* _104SharedBuffer;
    QTextStream* out;
    rtcm_t rtcm;
    QList<QPair<char*, QByteArray> > bufferList;
    QMutex bufferListMutex;
};

#endif // TRANSFERSERVER_H
