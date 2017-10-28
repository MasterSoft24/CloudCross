#ifndef MSSYNCTHREAD_H
#define MSSYNCTHREAD_H

#include <QObject>
#include "mscloudprovider.h"

class MSSyncThread : public QObject
{
    Q_OBJECT
public:
    explicit MSSyncThread(QObject *parent = 0, MSCloudProvider* p=0);

    MSCloudProvider* provider;
    QHash<QString,MSFSObject> threadSyncList;

signals:
    void finished();
public slots:
    void run();
};

#endif // MSSYNCTHREAD_H
