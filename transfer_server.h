#ifndef TRANSFERSERVER_H
#define TRANSFERSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>

class TransferServer : public QObject
{
    Q_OBJECT

public:
    explicit TransferServer(QObject* parent = 0);
    void tranferBuffer(int memID);

private slots:
    void incomingConnection();
    void handleConnected();
    void handleReceivedData();
    void handleError(QAbstractSocket::SocketError error);
    void handleDisconnect();

    void handle500MSTimer();

private:
    QTcpServer* server;
    QTcpSocket* socket;
    QMutex mutex;
    QByteArray startFlag;
    QByteArray endFlag;
    QByteArray data;
};

#endif // TRANSFERSERVER_H
