#include <QTimer>
#include <QDebug>
#include <QDateTime>

#include "transfer_client.h"
#include "common.h"
#include "transfer_config.h"

TransferClient::TransferClient(QObject* parent) :
    QObject(parent),
    socket(0), firstTransfer(false), flag_1(false), flag_2(false), flag_3(false), flag_4(false)
{
    int start = 0x73625342, end = 0x53427362;
    startFlag.append((char*) &start, sizeof(int));
    endFlag.append((char*) &end, sizeof(int));

    socket = new QTcpSocket();
    connect(socket, SIGNAL(connected()), this, SLOT(handleConnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceivedData()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
    //qRegisterMetaType<QAbstractSocket::SocketError>("SocketError");
    autoConnectTimer = new QTimer(this);
    connect(autoConnectTimer, SIGNAL(timeout()), this, SLOT(connectToServer()));
    autoConnectTimer->start(AUTO_CONNECT_INTERVAL);

    _104SharedBuffer = new SharedBuffer(SharedBuffer::LOOP_BUFFER,
                                        SharedBuffer::ONLY_WRITE,
                                        FindMemoryInfoFunc(104, TransferConfig::getMemLength(104)),
                                        TransferConfig::getMemLength(104),
                                        sizeof(UserHistoryLocationInfo));

    QTimer* _500MSTimer = new QTimer(this);
    connect(_500MSTimer, SIGNAL(timeout()), this, SLOT(handle500MSTimer()));
    _500MSTimer->start(10);

    QTimer* _1STimer = new QTimer(this);
    connect(_1STimer, SIGNAL(timeout()), this, SLOT(handle1STimer()));
    _1STimer->start(1000);

    QTimer* _1MINTimer = new QTimer(this);
    connect(_1MINTimer, SIGNAL(timeout()), this, SLOT(handle1MINTimer()));
    _1MINTimer->start(15 * 1000);

    QTimer* _10MINTimer = new QTimer(this);
    connect(_10MINTimer, SIGNAL(timeout()), this, SLOT(handle10MINTimer()));
    _10MINTimer->start(20 * 1000);
}

bool TransferClient::transferBuffer(int memID)
{
    int memLength = TransferConfig::getMemLength(memID);
    char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
    QByteArray data;
    data.append(pointer, memLength);
    if (memID > 6000 && memID < 6800) {
        if (*pointer == 1) {
            *pointer = 0;
        }
        else {
            return false;
        }
    }
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
    return true;
}

void TransferClient::connectToServer()
{
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        QString ip = TransferConfig::getServerIP();
        int port = TransferConfig::getPort();
        socket->connectToHost(ip, port);
    }
}

void TransferClient::handleConnected()
{
    qDebug() << "BufferTransfer:" << "Client socket connected.";
    if (autoConnectTimer->isActive()) {
        autoConnectTimer->stop();
    }
    *((bool*)FindMemoryInfoFunc(1604, 1)) = true;
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
    while (data.contains(startFlag) && data.contains(endFlag)) {
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
        if (qChecksum(memContent.data(), memContent.length()) != *(quint16*)(data.data() + endIndex - 2)) {
            qDebug() << "BufferTransfer:" << "CRC32 check error." << memID;
        }

        if (memID == 101) {
            char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
            int count = *((int*) pointer);
            for (int i = 0; i < count; i++) {
                Terminal* terminal = (Terminal*)(pointer + 4 + i * sizeof(Terminal));
                terminal->status = false;
            }
            qMemCopy(pointer, memContent.data(), memContent.length());
        }
        else if (memID == 104) {
            _104SharedBuffer->writeData(memContent.data(), memContent.length());
        }
        else {
            if (memContent.length() != memLength) {
                qDebug() << "BufferTransfer:" << "Error mem length." << memLength << memContent.length();
            }
            char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
            qMemCopy(pointer, memContent.data(), memLength);
        }
        data.remove(0, endIndex + 4);
    }
}

void TransferClient::handleError(QAbstractSocket::SocketError)
{
    qDebug() << "BufferTransfer:" << "Client socket error:" << socket->errorString();
    *((bool*)FindMemoryInfoFunc(1604, 1)) = false;
    if (!autoConnectTimer->isActive()) {
        qDebug() << "BufferTransfer:" << "Client socket reconnecting.";
        autoConnectTimer->start(AUTO_CONNECT_INTERVAL);
    }
}

void TransferClient::handleDisconnect()
{
    qDebug() << "BufferTransfer:" << "Client socket disconnect from server.";
    *((bool*)FindMemoryInfoFunc(1604, 1)) = false;
    if (!autoConnectTimer->isActive()) {
        qDebug() << "BufferTransfer:" << "Client socket reconnecting.";
        autoConnectTimer->start(AUTO_CONNECT_INTERVAL);
    }
}

void TransferClient::handle500MSTimer()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        void* receiverConfigPointer = FindMemoryInfoFunc(1601, TransferConfig::getMemLength(1601));
        int receiverAmount = *(int*) receiverConfigPointer;
        if (receiverAmount != 0) {
            ReceiverInBuffer* receivers = (ReceiverInBuffer*)(receiverConfigPointer + 4);
            //qMemCopy(receivers, receiverConfigPointer + 4, sizeof(ReceiverInBuffer) * receiverAmount);
            for (int i = 0; i < receiverAmount; i++) {
                if (transferBuffer(6000 + receivers[i].memID % 200) && i == 0) {
                    flag_1 = true;
                }
                if (transferBuffer(6200 + receivers[i].memID % 200) && i == 0) {
                    flag_2 = true;
                }
                if (transferBuffer(6400 + receivers[i].memID % 200) && i == 0) {
                    flag_3 = true;
                }
                if (transferBuffer(6600 + receivers[i].memID % 200) && i == 0) {
                    flag_4 = true;
                }
            }
        }
    }
}

void TransferClient::handle1STimer()
{
    if (!flag_1) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "No 1004 data";
    }
    if (!flag_2) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "No 1012 data";
    }
    if (!flag_3) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "No 1124 data";
    }
    flag_1 = flag_2 = flag_3 = false;
}

void TransferClient::handle1MINTimer()
{
    if (!flag_4) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "No 1005 data";
    }
    flag_4 = false;
    if (socket->state() == QAbstractSocket::ConnectedState) {
        transferBuffer(1501);
        transferBuffer(1601);
    }
}

void TransferClient::handle10MINTimer()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        transferBuffer(819);
        transferBuffer(820);
    }
}

