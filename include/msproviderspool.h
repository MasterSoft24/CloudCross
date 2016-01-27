#ifndef MSPROVIDERSPOOL_H
#define MSPROVIDERSPOOL_H

#include <QObject>
#include "qstdout.h"
#include "src/GoogleDrive/msgoogledrive.h"



class MSProvidersPool
{

private:




public:
    MSProvidersPool();

    QList<MSCloudProvider*> pool;

    MSCloudProvider* getProvider(QString providerName)    ;

    QString currentPath;// directory where program was run
    QString workPath;// path set with -p option

    MSCloudProvider::SyncStrategy strategy; // sync strategy

    QHash<QString,bool>flags;



    void addProvider(MSCloudProvider* provider);

    void getCurrentPath();
    void setWorkPath(QString path);

    void saveTokenFile(QString providerName);

    void loadTokenFile(QString providerName);

    void refreshToken(QString providerName);

    void setStrategy(MSCloudProvider::SyncStrategy strategy);

    void setFlag(QString flagName,bool flagVal);

};

#endif // MSPROVIDERSPOOL_H
