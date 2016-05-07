#ifndef MSYANDEXDISK_H
#define MSYANDEXDISK_H
#include <include/mscloudprovider.h>
#include <QFile>
#include <QDir>
#include <QFileDevice>

#include <sys/types.h>
#include <utime.h>
#include <sys/time.h>

class MSYandexDisk : public MSCloudProvider
{
public:
    MSYandexDisk();




     //=== REMOTE FUNCTIONS BLOCK ===

     // download file from cloud
     void remote_file_get(MSFSObject* object);
     // upload new file to cloud
     void remote_file_insert(MSFSObject* object);
     // update existing file on cloud
     void remote_file_update(MSFSObject* object);
     // Generates a set of file IDs which can be provided in insert requests
     bool remote_file_generateIDs(int count);
     // create folder on remote
     void remote_file_makeFolder(MSFSObject* object);
     void remote_file_makeFolder(MSFSObject* object,QString parentID);
     // trash file or folder on remote
     void remote_file_trash(MSFSObject* object);
     // create directory on remote, recursively if nesessary
     void remote_createDirectory(QString path);



     //=== LOCAL FUNCTION BLOCK ===

     // create directory on local, recursively if nesessary
     void local_createDirectory(QString path);
     void local_removeFile(QString path);
     void local_removeFolder(QString path);
     bool local_writeFileContent(QString filePath, QString  hrefToDownload);



     bool auth();
     void saveTokenFile(QString path) ;
     bool loadTokenFile(QString path);
     void loadStateFile();
     bool refreshToken();
     MSFSObject::ObjectState filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote);
     void doSync();


     QHash<QString,MSFSObject>   filelist_getFSObjectsByState(MSFSObject::ObjectState state) ;
     QHash<QString,MSFSObject>   filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state) ;
     QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type);
     QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type);
     bool                        filelist_FSObjectHasParent(MSFSObject fsObject);
     MSFSObject                  filelist_getParentFSObject(MSFSObject fsObject);
     void                        filelist_populateChanges(MSFSObject changedFSObject);



     bool testReplyBodyForError(QString body) ;
     QString getReplyErrorString(QString body) ;


     void createHashFromRemote();
     void readRemote(QString currentPath);//QString parentId,QString currentPath
     void readLocal(QString path);

     bool isFolder(QJsonValue remoteObject);
     bool isFile(QJsonValue remoteObject);
     bool filterServiceFileNames(QString path);
     bool filterIncludeFileNames(QString path);
     bool filterExcludeFileNames(QString path);

     void createSyncFileList();

     // sync local and remote filesystems hash table
     QHash<QString,MSFSObject> syncFileList;


     void directUpload(QString url,QString remotePath);



};

#endif // MSYANDEXDISK_H
