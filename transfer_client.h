#ifndef TRANSFERCLIENT_H
#define TRANSFERCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QTimer>

#include "shared_buffer.h"

class TransferClient : public QObject
{
    Q_OBJECT

public:
    explicit TransferClient(QObject *parent = 0);
    bool transferBuffer(int memID);

private slots:
    void connectToServer();
    void handleConnected();
    void handleReceivedData();
    void handleError(QAbstractSocket::SocketError error);
    void handleDisconnect();

    void handle500MSTimer();
    void handle1STimer();
    void handle1MINTimer();
    void handle10MINTimer();

private:
    QTcpSocket* socket;
    QMutex mutex;
    QByteArray startFlag;
    QByteArray endFlag;
    QByteArray data;
    bool firstTransfer;
    QTimer* autoConnectTimer;
    bool flag_1, flag_2, flag_3, flag_4;
    SharedBuffer* _104SharedBuffer;
};

#endif // TRANSFERCLIENT_H
