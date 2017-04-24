#ifndef TRANSFERCLIENT_H
#define TRANSFERCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QMutex>

class TransferClient : public QObject
{
    Q_OBJECT

public:
    explicit TransferClient(QObject *parent = 0);
    void transferBuffer(int memID);

private slots:
    void handleConnected();
    void handleReceivedData();
    void handleError(QAbstractSocket::SocketError error);
    void handleDisconnect();

    void handle500MSTimer();
    void handle1MINTimer();
    void handle10MINTimer();

private:
    QTcpSocket* socket;
    QMutex mutex;
    QByteArray startFlag;
    QByteArray endFlag;
    QByteArray data;
    bool firstTransfer;
    int i;
};

#endif // TRANSFERCLIENT_H
