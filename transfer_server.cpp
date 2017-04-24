#include <QTimer>
#include <QSettings>

#include "transfer_server.h"
#include "common.h"
#include "transfer_config.h"

TransferServer::TransferServer(QObject* parent):
    QObject(parent),
    server(0), socket(0)
{
    int start = 0x2A2A2A2A, end = 0x23232323;
    startFlag.append((char*) &start, sizeof(int));
    endFlag.append((char*) &end, sizeof(int));

    server = new QTcpServer(this);
    if (server->listen(QHostAddress::Any, TransferConfig::getPort())) {
        qDebug() << "BufferTransfer:" << "Server now listening.";
        connect(server, SIGNAL(newConnection()), this, SLOT(incomingConnection()));
    }
    else {
        qDebug() << "BufferTransfer:" << "Server listen error:" <<server->errorString();
    }

    QTimer* _500MSTimer = new QTimer();
    connect(_500MSTimer, SIGNAL(timeout()), this, SLOT(handle500MSTimer()));
    _500MSTimer->start(500);
}

void TransferServer::tranferBuffer(int memID)
{
    int memLength = TransferConfig::getMemLength(memID);
    char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
    QByteArray data;
    data.append(pointer, memLength);
    quint16 CRC16Code = qChecksum(data.data(), data.length());
    data.prepend((char*) &memID, sizeof(memID));
    data.prepend(startFlag);
    data.append((char*) &CRC16Code, sizeof(CRC16Code));
    data.append(endFlag);
    mutex.lock();
    forever {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            break;
        }
    }
    socket->write(data);
    socket->flush();
    mutex.unlock();
}

void TransferServer::incomingConnection()
{
    socket = server->nextPendingConnection();
    qDebug() << "BufferTransfer:" << "Server accept client connection.";
    connect(socket, SIGNAL(connected()), this, SLOT(handleConnected()), Qt::QueuedConnection);
    connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceivedData()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
}

void TransferServer::handleConnected()
{
    qDebug() << "BufferTransfer:" << "Server socket connected.";
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));
    data.clear();
}

void TransferServer::handleReceivedData()
{
    data.append(socket->readAll());
    if (data.contains(startFlag) && data.contains(endFlag)) {
        int startIndex = data.indexOf(startFlag);
        int endIndex = data.indexOf(endFlag);
        if (startIndex > endIndex) {
            qDebug() << "BufferTransfer:" << "Error index sequence.";
            data.remove(0, endIndex + 4);
            return;
        }
        if (startIndex != 0) {
            qDebug() << "BufferTransfer:" << "Error start index.";
        }
        int memID = *(int*)(data.data() + startIndex + 4);
        int memLength = TransferConfig::getMemLength(memID);
        QByteArray memContent = data.mid(startIndex + 8, endIndex - 10);
        if (memContent.length() != memLength) {
            qDebug() << "BufferTransfer:" << "Error mem length.";
        }
        if (qChecksum(memContent.data(), memContent.length()) != *(quint16*)(data.data() + endIndex - 2)) {
            qDebug() << "BufferTransfer:" << "CRC32 check error.";
        }
        char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
        qMemCopy(pointer, memContent.data(), memLength);
        data.remove(0, endIndex + 4);
    }
}

void TransferServer::handleError(QAbstractSocket::SocketError)
{
    qDebug() << "BufferTransfer:" << "Client socket error:" << socket->errorString();
}

void TransferServer::handleDisconnect()
{
    qDebug() << "BufferTransfer:" << "Client socket disconnect from server.";
}

void TransferServer::handle500MSTimer()
{
    if (socket != 0 && socket->state() == QAbstractSocket::ConnectedState) {
        tranferBuffer(101);
    }
}
