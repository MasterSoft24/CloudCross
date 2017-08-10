#ifndef LIBFUSECC_H
#define LIBFUSECC_H

#include "libfusecc_global.h"
#include <QDebug>
#include <QThread>
#include <QCoreApplication>



#include "include/msrequest.h"
#include "include/msproviderspool.h"


#include "ccseparatethread.h"




class LIBFUSECCSHARED_EXPORT libFuseCC : public QObject
{

    Q_OBJECT

public:
    libFuseCC(); // constructor

    bool getProviderInstance(ProviderType p, MSCloudProvider** lpProvider, const QString &workPath);
    bool getProviderInstance(const QString &providerName, MSCloudProvider** lpProvider, const QString &workPath);

    bool readRemoteFileList(MSCloudProvider* p);
    bool readLocalFileList(MSCloudProvider* p, const QString workPath);
    bool readSingleLocalFile(MSCloudProvider* p, const QString &pathToFile, const QString workPath);

    bool readFileContent(MSCloudProvider* p, const QString &destPath, MSFSObject obj);


    void getSeparateThreadInstance(CCSeparateThread** lpThread);

    void runInSeparateThread(MSCloudProvider* providerInstance,const QString command, const QMap<QString,QVariant> parms);
    void run(MSCloudProvider* providerInstance,const QString command, const QMap<QString,QVariant> parms);

    void clearLocalPartOfSyncFileList(MSCloudProvider *providerInstance);

private:

    static int argc ;
    static char * argv[];
    static QCoreApplication * app ;
    static QThread * thread ;

private slots:

    void onStarted();
};

#include "ccseparatethread.h"

#endif // LIBFUSECC_H
