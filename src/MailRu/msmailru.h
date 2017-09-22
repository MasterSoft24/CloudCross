/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.
    All rights reserved.


  BSD License

  Redistribution and use in source and binary forms, with or without modification, are
  permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice, this list
    of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.
  - Neither the name of the "Vladimir Kamensky" or "Master Soft LLC." nor the names of
    its contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY E
  XPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES O
  F MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SH
  ALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENT
  AL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROC
  UREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS I
  NTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRI
  CT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF T
  HE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef MSMAILRU_H
#define MSMAILRU_H

#include <include/mscloudprovider.h>
#include "include/qmultibuffer.h"
#include "include/mssyncthread.h"
#include <QFile>
#include <QDir>
#include <QFileDevice>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QBuffer>

#include <sys/types.h>
#include <utime.h>
#include <sys/time.h>

#ifdef CCROSS_LIB
#define QNetworkCookieJar MSNetworkCookieJar
//#define manager->cookieJar "getCookieJar"
#endif
#include <QNetworkCookieJar>

class MSMailRu : public MSCloudProvider
{
public:
    MSMailRu();

#ifdef CCROSS_LIB
   QHash<QString,MSNetworkCookieJar*> cookieList;
#endif



    //=== REMOTE FUNCTIONS BLOCK ===

    // download file from cloud
    bool remote_file_get(MSFSObject* object);
    // upload new file to cloud
    bool remote_file_insert(MSFSObject* object);
    // update existing file on cloud
    bool remote_file_update(MSFSObject* object);
    // Generates a set of file IDs which can be provided in insert requests
    bool remote_file_generateIDs(int count);
    // create folder on remote
    bool remote_file_makeFolder(MSFSObject* object);
    void remote_file_makeFolder(MSFSObject* object, const QString &parentID);
    // trash file or folder on remote
    bool remote_file_trash(MSFSObject* object);
    // create directory on remote, recursively if nesessary
    bool remote_createDirectory(const QString &path);
    // request for various url's
    QString remote_dispatcher(const QString &target);



    //=== LOCAL FUNCTION BLOCK ===

    // create directory on local, recursively if nesessary
    void local_createDirectory(const QString &path);
    void local_removeFile(const QString &path);
    void local_removeFolder(const QString &path);



    bool auth();
    void saveTokenFile(const QString &path) ;
    bool loadTokenFile(const QString &path);
    void loadStateFile();
    void saveStateFile();
    bool refreshToken();
    MSFSObject::ObjectState filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote);
    void checkFolderStructures();
    void doSync(QHash<QString,MSFSObject> fsObjectList);


    QHash<QString,MSFSObject>   filelist_getFSObjectsByState(MSFSObject::ObjectState state) ;
    QHash<QString,MSFSObject>   filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state) ;
    QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type);
    QHash<QString,MSFSObject>   filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type);
    bool                        filelist_FSObjectHasParent(const MSFSObject &fsObject);
    MSFSObject                  filelist_getParentFSObject(const MSFSObject &fsObject);
    void                        filelist_populateChanges(const MSFSObject &changedFSObject);



    bool testReplyBodyForError(const QString &body) ;
    QString getReplyErrorString(const QString &body) ;


    bool createHashFromRemote();
    bool readRemote(const QString &path, QNetworkCookieJar *cookie);//QString parentId,QString currentPath
    bool _readRemote(const QString &rootPath);
    bool readLocal(const QString &path);
    bool readLocalSingle(const QString &path);

    bool isFolder(const QJsonValue &remoteObject);
    bool isFile(const QJsonValue &remoteObject);

    bool createSyncFileList();

    // sync local and remote filesystems hash table
    //QHash<QString,MSFSObject> syncFileList;

    bool directUpload(const QString &url, const QString &remotePath);

     QString getInfo(); // get info about cloud



    // MAIL.RU ONLY

    QString login;
    QString password;

    QString build;
    QString x_page_id;
    QString email;
    QString x_email;
    QString api;

    QNetworkCookieJar* cookies;


public slots:

   bool onAuthFinished(const QString &html, MSCloudProvider *provider);



};

#endif // MSMAILRU_H
