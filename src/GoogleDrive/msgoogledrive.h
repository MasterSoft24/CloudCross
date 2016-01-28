/*
    CloudCross: Opensource program for syncronization of local files and folders with Google Drive

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation version 2
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef MSGOOGLEDRIVE_H
#define MSGOOGLEDRIVE_H

#include <QObject>
#include <include/mscloudprovider.h>


class MSGoogleDrive : public MSCloudProvider
{

private:

    // create hash table from remote json file records
    void createHashFromRemote();
    // get slice from remote files hash table by parent and some other conditions
    QHash<QString,QJsonValue> get(QString parentId, int target);
    bool isFile(QJsonValue remoteObject);
    bool isFolder(QJsonValue remoteObject);


    QString getRoot();// return root id  of google drive


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



    void readRemote(QString parentId, QString currentPath);
    void readLocal(QString path);

public:
    MSGoogleDrive();

     void saveTokenFile(QString path);  // reimplemented from MSCloudProvider
     bool loadTokenFile(QString path);  // reimplemented from MSCloudProvider
     void refreshToken();               // reimplemented from MSCloudProvider
     void loadStateFile();              // reimplemented from MSCloudProvider

     void createSyncFileList();

     QHash<QString,MSFSObject> getRemoteFileList();

     bool filterServiceFileNames(QString path);
     bool filterIncludeFileNames(QString path);
     bool filterExcludeFileNames(QString path);


     void doSync();

     QHash<QString,MSFSObject>      filelist_getFSObjectsByState(MSFSObject::ObjectState state);

     QHash<QString,MSFSObject>      filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state);

     QHash<QString,MSFSObject>      filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type);

     QHash<QString,MSFSObject>      filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type);

     MSFSObject::ObjectState        filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote);

     bool                           filelist_FSObjectHasParent(MSFSObject fsObject);

     MSFSObject                     filelist_getParentFSObject(MSFSObject fsObject);

     void                           filelist_populateChanges(MSFSObject changedFSObject);

     // remote files hash table
     QHash<QString,QJsonValue>driveJSONFileList;

     // sync local and remote filesystems hash table
     QHash<QString,MSFSObject> syncFileList;


};

#endif // MSGOOGLEDRIVE_H
