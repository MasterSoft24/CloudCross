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
#include <QRegularExpression>

#include "msfsobject.h"
#include "msidslist.h"
#include "msrequest.h"
#include "qstdout.h"


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



    virtual bool auth() = 0;
    virtual void saveTokenFile(QString path) =0 ;
    virtual bool loadTokenFile(QString path)=0;
    virtual void loadStateFile()=0;
    virtual bool refreshToken()=0;
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


    bool local_writeFileContent(QString filePath, MSRequest *req);


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
    QHash<QString,QString>options;

    QString includeList;
    QString excludeList;

    void setFlag(QString flagName, bool flagVal);
    bool getFlag(QString flagName);

    QString getOption(QString optionName);

    virtual bool testReplyBodyForError(QString body) = 0;
    virtual QString getReplyErrorString(QString body) = 0;

    virtual void directUpload(QString url) =0;

};


#endif // MSCLOUDPROVIDER_H
