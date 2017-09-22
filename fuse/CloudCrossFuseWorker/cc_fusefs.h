#ifndef CC_FUSEFS_H
#define CC_FUSEFS_H



#include<pthread.h>
#include <unistd.h>
//#include <chrono>

#include <QObject>
#include <QTimer>
#include <QDateTime>

#include "include/msrequest.h"



#include "../libfusecc/libfusecc.h"
#include "../libfusecc/ccseparatethread.h"
#include "include/msproviderspool.h"

class CC_FuseFS : public QObject
{
     Q_OBJECT
private:

    //static std::thread syncTimer;
    pthread_t tid;

public:
    CC_FuseFS();

    static CC_FuseFS *_instance;

    static CC_FuseFS *Instance();

    static void doSheduleUpdate();


    void log(QString mes);




public:

    //static QString TTTT;

    static QString mountPath;
    static QString tokenPath;
    //static QString credentialsPath;
    static ProviderType provider;
    static QString providerName;
    static libFuseCC* ccLib;
    static MSCloudProvider* providerObject;

    static bool updateSheduled;
    static int lastUpdateSheduled;
    static bool fsBlocked;

public slots:

    //void *onSyncTimer(void *p);




signals:


};

#endif // CC_FUSEFS_H
