#ifndef MSCLOUDPROVIDER_H
#define MSCLOUDPROVIDER_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfoList>
#include <QTextStream>
#include <QDataStream>
#include <QCryptographicHash>
#include <QList>

#include "msfsobject.h"
#include "msidslist.h"

//#define StdOut MSCloudProvider::qStdOut()



//class iMSCloudProvider{
//    virtual void saveTokenFile(QString path) =0;

//};


class MSCloudProvider
{

private:




public:

    QString currentPath;// directory where program was run
    QString workPath;// path set with -p option

    enum CloudObjects{
                        Files=1,
                        Folders=2,
                        FilesAndFolders=3,
                        NoTrash=4
    };

    enum SyncStrategy{
                        PreferLocal=1,
                        PreferRemote=2
    };

    MSCloudProvider();

    QString providerName;

    QString tokenFileName;
    QString stateFileName;
    QString trashFileName;

    QString token;
    QString access_token;

    qint64 lastSyncTime;// date and time of last sync

    SyncStrategy strategy; // sync strategy


    MSIDsList ids_list;




    virtual void saveTokenFile(QString path) =0 ;
    virtual void loadTokenFile(QString path)=0;
    virtual void loadStateFile()=0;
    virtual void refreshToken()=0;
    virtual MSFSObject::ObjectState filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote)=0;
    virtual void doSync()=0;
    virtual bool remote_file_generateIDs(int count) =0 ;

    virtual QHash<QString,MSFSObject>   filelist_getFSObjectsByState(MSFSObject::ObjectState state) =0;
    virtual QHash<QString,MSFSObject>   filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state) =0;
    virtual QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type)=0;
    virtual QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type)=0;
    virtual bool                        filelist_FSObjectHasParent(MSFSObject fsObject)=0;
    virtual MSFSObject                  filelist_getParentFSObject(MSFSObject fsObject)=0;
    virtual void                        filelist_populateChanges(MSFSObject changedFSObject)=0;
//    virtual void                        filelist_populateChanges(QHash<QString,MSFSObject> changedFSObjectList)=0;




    QString fileChecksum(QString &fileName, QCryptographicHash::Algorithm hashAlgorithm);

    // converts to milliseconds in UTC timezone
    qint64 toMilliseconds(QDateTime dateTime, bool isUTC=false);
    qint64 toMilliseconds(QString dateTimeString, bool isUTC=false);


    // Modify Mode Flags

    bool useInclude;
    bool dryRun;
    bool noHidden;
    bool newRev; //???????

    QHash<QString,bool>flags;

    QString includeList;
    QString excludeList;

    void setFlag(QString flagName, bool flagVal);
    bool getFlag(QString flagName);

};


#endif // MSCLOUDPROVIDER_H
