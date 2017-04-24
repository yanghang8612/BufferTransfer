#include <QThread>
#include <QDebug>

#include "transfer_client.h"
#include "transfer_server.h"

class ServerWriter : public QThread {
    void run() {
        TransferServer* server = new TransferServer();
        exec();
    }
};

class ClientWriter : public QThread {
    void run() {
        TransferClient* client = new TransferClient();
        exec();
    }
};

QThread* transferThread;

extern "C" bool DllMain(int args, char* argv[])
{
    qDebug() << "BufferTransfer:" << "DllMain function called";

    if (args > 0 && qstrcmp(argv[0], "Server") == 0) {
        transferThread = new ServerWriter();
        transferThread->start();
    }
    else if (args > 0 && qstrcmp(argv[0], "Client") == 0) {
        transferThread = new ClientWriter();
        transferThread->start();
    }
    else {
        qDebug() << "BufferTransfer:" << "No appropriate deployment type according to parameter 'argv'";
        return false;
    }

    return true;
}

extern "C" bool DllInit(int, char*)
{
    qDebug() << "BufferTransfer:" << "DllInit function called";
    return true;
}

extern "C" bool DllStart()
{
    qDebug() << "BufferTransfer:" << "DllStart function called";
    return true;
}

extern "C" bool DllStop()
{
    qDebug() << "BufferTransfer:" << "DllStop function called";
    transferThread->terminate();
    return true;
}

extern "C" bool DllContraryInit()
{
    qDebug() << "BufferTransfer:" << "DllContraryInit function called";
    transferThread->deleteLater();
    return true;
}
