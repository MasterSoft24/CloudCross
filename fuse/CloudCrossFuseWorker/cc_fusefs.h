#ifndef CC_FUSEFS_H
#define CC_FUSEFS_H





#include <QObject>

#include "include/msrequest.h"


#include "../libfusecc/libfusecc.h"
#include "../libfusecc/ccseparatethread.h"
#include "include/msproviderspool.h"

class CC_FuseFS : public QObject
{
     Q_OBJECT

public:
    CC_FuseFS();

    static CC_FuseFS *_instance;

    static CC_FuseFS *Instance();


    void log(QString mes);

    int Open(const char *path, struct fuse_file_info *fileInfo);


public:

    //static QString TTTT;

    static QString mountPath;
    static QString tokenPath;
    static ProviderType provider;
    static QString providerName;
    static libFuseCC* ccLib;
    static MSCloudProvider* providerObject;


public slots:

    void onTestSig();




signals:

    void testSig();
};

#endif // CC_FUSEFS_H
