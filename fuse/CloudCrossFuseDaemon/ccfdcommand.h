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


//#include "include/mscloudprovider.h"
#include "include/msrequest.h"
#include "include/msproviderspool.h"

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

    QLocalSocket *clientConnection;
    QHash<QString,MSFSObject> fileList;

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
};

#endif // CCFDCOMMAND_H
