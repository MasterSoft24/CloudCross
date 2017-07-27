#ifndef CCSEPARATETHREAD_H
#define CCSEPARATETHREAD_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QDebug>
#include "include/msproviderspool.h"

class libFuseCC;

class CCSeparateThread : public QObject
{
    Q_OBJECT
public:
    explicit CCSeparateThread(QObject *parent = 0);

    libFuseCC* lpFuseCC;
    MSCloudProvider* lpProviderObject;

    QString commandToExecute;
    QMap<QString,QVariant> commandParameters;


private:

    bool command_getRemoteFileList();
    bool command_getFileContent();

    void log(QString mes);

signals:

    void finished();

public slots:

    void run();


};

#endif // CCSEPARATETHREAD_H
