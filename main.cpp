#include <QCoreApplication>
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QThreadPool>
#include <QDebug>

#include "common.h"
#include "transfer_client.h"
#include "transfer_server.h"

FINDMEMORYINFOFUNC FindMemoryInfoFunc;

void* find(int memID, int memLength) {
    static QMap<int, void*> map;
    if (!map.contains(memID)) {
        void* pointer = malloc(memLength);
        qMemSet(pointer, 0, memLength);
        map.insert(memID, pointer);
    }
    return map[memID];
}

class Writer : public QThread {
    void run() {
        TransferServer* server = new TransferServer();
        //TransferClient* client = new TransferClient();
        exec();
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    FindMemoryInfoFunc = find;

    Writer* writer = new Writer();
    writer->start();

    //QThreadPool::globalInstance()->start(new Writer());

    return a.exec();
}
