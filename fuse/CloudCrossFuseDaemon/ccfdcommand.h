#ifndef CCFDCOMMAND_H
#define CCFDCOMMAND_H


#include <QObject>
#include <QStringList>
#include<QtNetwork/QLocalSocket>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QLocalServer>
#include<QtNetwork/QLocalSocket>
#include <QThread>
#include <QProcess>
#include <QFileSystemWatcher>

//#include "include/mscloudprovider.h"
#include "include/msrequest.h"
#include "include/msproviderspool.h"


#include <iostream>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <vector>
#include <poll.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

typedef struct _fswatcher_param{

    //void (*sendCommand) (json command);
    bool state; // state of thread (true means a thread runned)

    int wd; // watcher descriptor
    int watcher;

    QString path; // path to mounted dir
    QString socket;
    QString provider;
    QString credPath; // path to credentials

}fswatcher_param;





enum ProviderType{

    Google,
    Dropbox,
    Yandex,
    Mailru,
    OneDrive
};

typedef struct _fuse_worker{

    QLocalServer* worker_soket;
    QProcess* worker;
    QString socket_name;
    QString provider;
    QString workPath;
    QString mountPoint;

    QLocalSocket *clientConnection;
    QHash<QString,MSFSObject> fileList;



    bool workerLocked;
    bool updateSheduled;
    int lastUpdateSheduled;

    fswatcher_param* watcherStruct;

    QStringList removedItems;

}fuse_worker;












class CCFDCommand : public QObject
{
    Q_OBJECT
public:
    explicit CCFDCommand(QObject *parent = 0);


    void parseParameters();

    QJsonObject getRetStub();
    void log(QString mes);
    QJsonObject FSObjectToJSON(const MSFSObject &obj);

    void executor(MSCloudProvider* p);

    QJsonObject params;
    QLocalSocket* socket;
    QString command;
    QString socket_name;
    QString path;
    ProviderType provider;

    fuse_worker* workerPtr;



signals:
    void finished();
    void result(const QString& r);

public slots:

    void run();

//    void watcherRemovedSlot();
//    void watcherModifiedSlot();
//    void watcherCreatedSlot();

//    void watcherOnFileChanged(QString file);
//    void watcherOnDirectoryChanged(QString dir);

};

#endif // CCFDCOMMAND_H
