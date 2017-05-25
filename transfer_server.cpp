#include <QThread>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QDateTime>
#include <QFile>

#include "transfer_server.h"
#include "common.h"
#include "transfer_config.h"

class WriteBufferTask : public QThread
{
public:
    WriteBufferTask(QList<QPair<char*, QByteArray> >& bufferList, QMutex& bufferListMutex) :
        bufferList(bufferList), bufferListMutex(bufferListMutex)
    {}

    void run()
    {
        forever {
            bufferListMutex.lock();
            QMutableListIterator <QPair<char*, QByteArray> > iterator(bufferList);
            while (iterator.hasNext()) {
                QPair<char*, QByteArray> item = iterator.next();
                if (*item.first == 0) {
                    qMemCopy(item.first, item.second.data(), item.second.length());
                    iterator.remove();
                }
            }
            bufferListMutex.unlock();
            msleep(10);
        }
    }
private:
    QList<QPair<char*, QByteArray> >& bufferList;
    QMutex& bufferListMutex;
};

TransferServer::TransferServer(QObject* parent):
    QObject(parent),
    server(0), socket(0), flag_1(false),  flag_2(false),  flag_3(false)
{
//    QThread* thread = new WriteBufferTask(bufferList, bufferListMutex);
//    thread->start();

//    QFile* file = new QFile("record");
//    file->open(QIODevice::WriteOnly | QIODevice::Text);
//    out = new QTextStream(file);
//    init_rtcm(&rtcm);

    int start = 0x73625342, end = 0x53427362;
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

    _104SharedBuffer = new SharedBuffer(SharedBuffer::LOOP_BUFFER,
                                        SharedBuffer::ONLY_READ,
                                        FindMemoryInfoFunc(104, TransferConfig::getMemLength(104)),
                                        TransferConfig::getMemLength(104),
                                        sizeof(UserHistoryLocationInfo));

    QTimer* _500MSTimer = new QTimer();
    connect(_500MSTimer, SIGNAL(timeout()), this, SLOT(handle500MSTimer()));
    _500MSTimer->start(500);

    QTimer* _1STimer = new QTimer();
    connect(_1STimer, SIGNAL(timeout()), this, SLOT(handle1STimer()));
    //_1STimer->start(1000);
}

void TransferServer::tranferBuffer(const char* start, int length, int memID)
{
    QByteArray data;
    data.append(start, length);
    quint16 CRC16Code = qChecksum(data.data(), data.length());
    data.prepend((char*) &memID, sizeof(memID));
    data.prepend(startFlag);
    data.append((char*) &CRC16Code, sizeof(CRC16Code));
    data.append(endFlag);
    socketWriteMutex.lock();
    forever {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            break;
        }
    }
    socket->write(data);
    socket->flush();
    socketWriteMutex.unlock();
}

void TransferServer::incomingConnection()
{
    QTcpSocket* newClient = server->nextPendingConnection();
    if (newClient->peerAddress().toString() == TransferConfig::getClientIP()) {
        qDebug() << "BufferTransfer:" << "Server accept client connection.";
        socket = newClient;
        socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));
        data.clear();
        *((bool*)FindMemoryInfoFunc(1604, 1)) = true;
        connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceivedData()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    }
    else {
        newClient->close();
        qDebug() << "BufferTransfer:" << "Server reject client connection.";
    }
}

void TransferServer::handleReceivedData()
{
    data.append(socket->readAll());
    //qDebug() << data.length();
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
        if (memContent.length() != memLength) {
            qDebug() << "BufferTransfer:" << "Error mem length." << memLength << memContent.length();
        }
        if (qChecksum(memContent.data(), memContent.length()) != *(quint16*)(data.data() + endIndex - 2)) {
            qDebug() << "BufferTransfer:" << "CRC32 check error." << memID;
        }
//        if (memID == 6004 || memID == 6204 || memID == 6404) {
//            int ret;
//            int dataLength = *((int*)(memContent.data() + 1));
//            for (int i = 0; i < dataLength; i++) {
//                ret = input_rtcm3(&rtcm, memContent.at(i + 5));
//            }
//            (*out) << memID << " " << time_str(rtcm.time, 2) << endl;
//            out->flush();
//        }
//        switch (memID) {
//            case 6004:
//                flag_1 = true;
//                break;
//            case 6204:
//                flag_2 = true;
//                break;
//            case 6404:
//                flag_3 = true;
//                break;
//            default:
//                break;
//        }
        char* pointer = (char*) FindMemoryInfoFunc(memID, memLength);
//        if (*pointer == 1 && memID > 6000 && memID < 6800) {
//            bufferListMutex.lock();
//            bufferList.append(QPair<char*, QByteArray>(pointer, memContent));
//            bufferListMutex.unlock();
//        }
//        else {
//            qMemCopy(pointer, memContent.data(), memContent.length());
//        }
        qMemCopy(pointer, memContent.data(), memContent.length());
        data.remove(0, endIndex + 4);
    }
}

void TransferServer::handleError(QAbstractSocket::SocketError)
{
    qDebug() << "BufferTransfer:" << "Client socket error:" << socket->errorString();
    *((bool*)FindMemoryInfoFunc(1604, 1)) = false;
}

void TransferServer::handleDisconnect()
{
    qDebug() << "BufferTransfer:" << "Client socket disconnect from server.";
    *((bool*)FindMemoryInfoFunc(1604, 1)) = false;
}

void TransferServer::handle500MSTimer()
{
    if (socket != 0 && socket->state() == QAbstractSocket::ConnectedState) {

        //101
        char* dataPointer = (char*) FindMemoryInfoFunc(101, TransferConfig::getMemLength(101));
        Terminal* terminal;
        QByteArray data;
        for (int i = 0; i < 2000; i++) {
            terminal = (Terminal*)(dataPointer + 4 + i * sizeof(Terminal));
            if (terminal->ip[0] != 0) {
                data.append((char*) terminal, sizeof(Terminal));
            }
        }
        data.prepend(dataPointer, 4);
        tranferBuffer(data.data(), data.length(), 101);

        //104
        static UserHistoryLocationInfo locationInfo[1000];
        int length = _104SharedBuffer->readData(locationInfo, sizeof(locationInfo));
        if (length != 0) {
            tranferBuffer((const char*)locationInfo, length, 104);
        }
    }
}

void TransferServer::handle1STimer()
{
    if (!flag_1) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "1004 not received.";
    }
    if (!flag_2) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "1012 not received.";
    }
    if (!flag_3) {
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss") << "1124 not received.";
    }
    flag_1 = flag_2 = flag_3 = false;
}
