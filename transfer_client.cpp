#include <QTimer>

#include "transfer_client.h"
#include "common.h"
#include "transfer_config.h"

TransferClient::TransferClient(QObject* parent) :
    QObject(parent),
    socket(0), firstTransfer(false)
{
    int start = 0x2A2A2A2A, end = 0x23232323;
    startFlag.append((char*) &start, sizeof(int));
    endFlag.append((char*) &end, sizeof(int));

    socket = new QTcpSocket();
    socket->connectToHost(TransferConfig::getServerIP(), TransferConfig::getPort());
    connect(socket, SIGNAL(connected()), this, SLOT(handleConnected()), Qt::QueuedConnection);
    connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceivedData()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
    //qRegisterMetaType<QAbstractSocket::SocketError>("SocketError");

    QTimer* _500MSTimer = new QTimer();
    connect(_500MSTimer, SIGNAL(timeout()), this, SLOT(handle500MSTimer()));
    _500MSTimer->start(500);

    QTimer* _1MINTimer = new QTimer();
    connect(_1MINTimer, SIGNAL(timeout()), this, SLOT(handle1MINTimer()));
    _1MINTimer->start(10 * 1000);

    QTimer* _10MINTimer = new QTimer();
    connect(_10MINTimer, SIGNAL(timeout()), this, SLOT(handle10MINTimer()));
    _10MINTimer->start(20 * 1000);
}

void TransferClient::transferBuffer(int memID)
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

void TransferClient::handleConnected()
{
    qDebug() << "BufferTransfer:" << "Client socket connected.";
    qDebug() << "BufferTransfer:" << i;
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));
    data.clear();
    if (!firstTransfer) {
        handle1MINTimer();
        handle10MINTimer();
    }
}

void TransferClient::handleReceivedData()
{
//    static QByteArray memID;
//    static QByteArray checkCode;
//    static int leftLength = 0;
//    static char* writePointer = 0;
//    static QByteArray data = socket->readAll();
//    while (data.length() > 0) {
//        if (memID.length() < 4) {
//            int memIDLeft = 4 - memID.length();
//            if (data.length() >= memIDLeft) {
//                memID.append(data.left(memIDLeft));
//                data.remove(0, memIDLeft);
//                leftLength = TransferConfig::getMemLength(memID.toInt());
//                writePointer = (char*) FindMemoryInfoFunc(memID.toInt(), leftLength);
//            }
//            else {
//                memID.append(data);
//            }
//        }
//        else if (memID.length() == 4) {
//            int dataToWrite = (leftLength > data.length()) ? data.length() : leftLength;
//            leftLength -= dataToWrite;
//            qMemCopy(writePointer, data.data(), dataToWrite);
//            writePointer += dataToWrite;
//            data.remove(0, dataToWrite);
//            if (leftLength == 0) {
//                memID.append('0');
//            }
//        }
//        else {
//            int codeLeft = 2 - checkCode.length();
//            if (data.length() >= codeLeft) {
//                checkCode.append(data.left(codeLeft));
//                data.remove(0, codeLeft);

//            }
//        }

//    }
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

void TransferClient::handleError(QAbstractSocket::SocketError)
{
    qDebug() << "BufferTransfer:" << "Client socket error:" << socket->errorString();
    qDebug() << "BufferTransfer:" << "Client socket reconnecting.";
    qDebug() << "BufferTransfer:" << i;
    socket->connectToHost(TransferConfig::getServerIP(), TransferConfig::getPort());
    socket->waitForConnected();
    i++;
}

void TransferClient::handleDisconnect()
{
    qDebug() << "BufferTransfer:" << "Client socket disconnect from server.";
    qDebug() << "BufferTransfer:" << "Client socket reconnecting.";
    socket->connectToHost(TransferConfig::getServerIP(), TransferConfig::getPort());
}

void TransferClient::handle500MSTimer()
{
    void* receiverConfigPointer = FindMemoryInfoFunc(1601, TransferConfig::getMemLength(1601));
    int receiverAmount = *(int*) receiverConfigPointer;
    if (receiverAmount != 0) {
        ReceiverInBuffer* receivers = (ReceiverInBuffer*)(receiverConfigPointer + 4);
        //qMemCopy(receivers, receiverConfigPointer + 4, sizeof(ReceiverInBuffer) * receiverAmount);
        for (int i = 0; i < receiverAmount; i++) {
            transferBuffer(6000 + receivers[i].memID % 200);
            transferBuffer(6200 + receivers[i].memID % 200);
            transferBuffer(6400 + receivers[i].memID % 200);
        }
    }
}

void TransferClient::handle1MINTimer()
{
    transferBuffer(1501);
    transferBuffer(1601);
}

void TransferClient::handle10MINTimer()
{
    transferBuffer(819);
    transferBuffer(820);
}

