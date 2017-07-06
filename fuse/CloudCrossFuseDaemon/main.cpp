#include <QCoreApplication>

#include <QDir>
#include <QLockFile>
#include "commandserver.h"

#define USG "ccfd-WhfNxPk-544h-gaQmZog39q"
#define LOCK_FILE "/ccfd.lock"


CommandServer* server;

int main(int argc, char *argv[])
{

    // single instance control
    QString tmpDir = QDir::tempPath();
    QLockFile lockFile(tmpDir + LOCK_FILE);

    if(!lockFile.tryLock(100)){

        return 1;
    }

    QCoreApplication a(argc, argv);


    server=new CommandServer();
    server->start();


    return a.exec();
}

//  nc -U /tmp/ccfd.sock

//  new_mount^4^/home/ms/QT/CloudCross/build/debug/TEST4^/tmp/example
