#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include <QObject>
#include <QtNetwork/QLocalServer>
#include<QtNetwork/QLocalSocket>
#include <QFile>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>


#include "ccfdcommand.h"



class CommandServer : public QObject
{
    Q_OBJECT
public:
    explicit CommandServer(QObject *parent = 0);


    QLocalServer* srv;
    QHash<QString,fuse_worker*> workersList;

    bool start();
    QString generateRandom(int count);

signals:

public slots:
    void onNewConnection();
    void onNewCommandRecieved();
    void onClientDisconnected();
    void onThreadResult(const QString& r);
};

#endif // COMMANDSERVER_H
