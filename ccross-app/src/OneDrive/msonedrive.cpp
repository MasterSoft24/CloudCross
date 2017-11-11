/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2017  Vladimir Kamensky
    Copyright (C) 2017  Master Soft LLC.
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
#include "msonedrive.h"

#include <utime.h> // for macOS



QByteArray excludeChars = "/:";

MSOneDrive::MSOneDrive():
    MSCloudProvider()

{
    this->providerName=     QStringLiteral("OneDrive");
    this->tokenFileName=    QStringLiteral(".ondr");
    this->stateFileName=    QStringLiteral(".ondr_state");
    this->trashFileName=    QStringLiteral(".trash_ondr");
}

bool MSOneDrive::remote_file_get(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    if(object->remote.data[QStringLiteral("id")].toString() == QStringLiteral(""))return false; //remove me

    if(object->remote.objectType == MSRemoteFSObject::Type::folder) return true;

    QString filePath = this->workPath + object->path + object->fileName;

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

afterReauth:

    req->setRequestUrl(object->remote.data[QStringLiteral("@content.downloadUrl")].toString());
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);

    req->setOutputFile(this->workPath + object->path + CCROSS_TMP_PREFIX +object->fileName);
    req->exec();


    QString c = req->readReplyText();

    if((int)req->replyError == 0){

        this->local_actualizeTempFile(this->workPath + object->path + CCROSS_TMP_PREFIX +object->fileName);

        utimbuf tb;

        QString dd=object->remote.data[QStringLiteral("fileSystemInfo")].toObject()[QLatin1String("lastModifiedDateTime")].toString();
        tb.actime=(this->toMilliseconds(dd,true))/1000;;
        tb.modtime=(this->toMilliseconds(dd,true))/1000;;


        utime(filePath.toLocal8Bit().constData(),&tb);

        delete(req);
        return true;

    }
    else{
        if(req->replyErrorText.contains(QStringLiteral("Host requires authentication"))){
            delete(req);
            this->refreshToken();
            req = new MSHttpRequest(this->proxyServer);

            qStdOut() << QStringLiteral("OneDrive token expired. Refreshing token done. Retry last operation. ") <<endl;

            goto afterReauth;
        }
    }

    return false;
}

bool MSOneDrive::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< QString(object->fileName + QStringLiteral(" is a folder. Skipped.")) <<endl ;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    // Create an upload session ===========

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

afterReauth:

    QString rpath = QString(object->path+object->fileName);
    req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root:")+ QString(QUrl::toPercentEncoding(rpath)) +QStringLiteral(":/upload.createSession"));
    req->setMethod("post");

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);

//    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));
//    req->addQueryItem("path",                           object->path+object->fileName);
//    req->addQueryItem("overwrite",                           "true");

    req->exec();

    if(!req->replyOK()){

        if(req->replyErrorText.contains(QStringLiteral("Host requires authentication"))){
            delete(req);
            this->refreshToken();
            req = new MSHttpRequest(this->proxyServer);

            qStdOut() << QStringLiteral("OneDrive token expired. Refreshing token done. Retry last operation. ")<<endl ;

            goto afterReauth;
        }

        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString uploadUrl=job[QStringLiteral("uploadUrl")].toString();

    QString filePath=this->workPath+object->path+object->fileName;

    if(uploadUrl == QStringLiteral("")){

        delete(req);
        qStdOut()<< QString(QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")) <<endl ;
        return false;
    }

    quint64 cursorPosition=0;



    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    quint64 passCount=(quint64)((double)fSize/(double)ONEDRIVE_CHUNK_SIZE);// count of 59MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QString(QStringLiteral("Unable to open of ")+filePath) <<endl ;
        delete(req);
        return false;
    }

    delete(req);

    if(passCount==0){// onepass and finalize uploading

        req = new MSHttpRequest(this->proxyServer);


        req->setRequestUrl(uploadUrl);
        req->setMethod(QStringLiteral("put"));

        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Length"),                    QString::number(fSize).toLocal8Bit());

        if(fSize >0){
            req->addHeader(QStringLiteral("Content-Range"),                     QStringLiteral("bytes 0-")+QString::number(fSize-1).toLocal8Bit()+QStringLiteral("/")+QString::number(fSize).toLocal8Bit());
        }
        else{
            qStdOut()<< QString(QStringLiteral("    OneDrive does not support zero-length files. Uploading skiped."))<<endl;
            delete(req);
            return false;
        }

        req->addHeader("Content-Type",                      QStringLiteral("")); //need for correct processing into MSHttpRequest

//        QByteArray* ba=new QByteArray();

////        file.seek(cursorPosition);
//        ba->append(file.read(fSize));
//        req->put(*ba);

//        req->put(file.read(ONEDRIVE_CHUNK_SIZE));

        req->setInputDataStream(&file);
        req->exec();

        if(!req->replyOK()){

            if(req->replyErrorText.contains(QStringLiteral("Host requires authentication"))){
                delete(req);
                this->refreshToken();
                req = new MSHttpRequest(this->proxyServer);
                qStdOut() << QString(QStringLiteral("OneDrive token expired. Refreshing token done. Retry last operation. "))<<endl ;

                goto afterReauth;
            }

            req->printReplyError();
            file.close();
            delete(req);
            return false;
        }

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject o = json.object();

        object->remote.fileSize=  o[QStringLiteral("size")].toInt();
        object->remote.data=o;
        object->remote.exist=true;
        object->isDocFormat=false;

        object->state=MSFSObject::ObjectState::Sync;

        object->remote.objectType=MSRemoteFSObject::Type::file;
        object->remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("lastModifiedDateTime")].toString(),true);
        object->remote.md5Hash= o[QStringLiteral("file")].toObject()[QStringLiteral("hashes")].toObject()[QStringLiteral("sha1Hash")].toString();

        this->filelist_populateChanges(*object);

        file.close();

        delete(req);
        return true;

    }
    else{ // multipass uploading

        do{

            req = new MSHttpRequest(this->proxyServer);
            req->setRequestUrl(uploadUrl);

            req->setMethod(QStringLiteral("put"));

            //set upload block size

            quint64 blsz=0;

            if(cursorPosition == 0){

                blsz=ONEDRIVE_CHUNK_SIZE;
            }
            else{

                if(fSize - cursorPosition < ONEDRIVE_CHUNK_SIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=ONEDRIVE_CHUNK_SIZE;
                }

            }

            //----------------------

            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Length"),                    QString::number(blsz).toLocal8Bit());

            QString t= QStringLiteral("bytes ")+QString::number(cursorPosition).toLocal8Bit()+QStringLiteral("-")+QString::number(cursorPosition+blsz-1).toLocal8Bit()+QStringLiteral("/")+QString::number(fSize).toLocal8Bit();

            req->addHeader(QStringLiteral("Content-Range"),                     QStringLiteral("bytes ")+QString::number(cursorPosition).toLocal8Bit()+QStringLiteral("-")+QString::number(cursorPosition+blsz-1).toLocal8Bit()+QStringLiteral("/")+QString::number(fSize).toLocal8Bit());

            //QByteArray* ba=new QByteArray();

//            file.seek(cursorPosition);
            //ba->append(file.read(ONEDRIVE_CHUNK_SIZE));

            req->addHeader("Content-Type",                      QStringLiteral("")); //need for correct processing into MSHttpRequest


            req->setInputDataStream(&file);
            req->setPayloadChunkData(blsz,cursorPosition);
//            req->put(file.read(ONEDRIVE_CHUNK_SIZE));
            req->exec();

            if(!req->replyOK()){

                if(req->replyErrorText.contains(QStringLiteral("Host requires authentication"))){
                    delete(req);
                    this->refreshToken();
                    req = new MSHttpRequest(this->proxyServer);
                    qStdOut() << QString(QStringLiteral("OneDrive token expired. Refreshing token done. Retry last operation. "))<<endl ;
                    goto afterReauth;
                }

                req->printReplyError();
                file.close();
                delete(req);
                return false;
            }

            QString content=req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject o = json.object();

            QString cp=o[QStringLiteral("nextExpectedRanges")].toArray()[0].toString().split(QStringLiteral("-"))[0];

            if(((int)req->replyError == 200)||((int)req->replyError == 201) || (cp == QStringLiteral("")) ){

                break;
            }

            cursorPosition=cp.toLong();

            delete(req);
            //delete(ba);

        }while(true);

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject o = json.object();

        object->remote.fileSize=  o[QStringLiteral("size")].toInt();
        object->remote.data=o;
        object->remote.exist=true;
        object->isDocFormat=false;

        object->state=MSFSObject::ObjectState::Sync;

        object->remote.objectType=MSRemoteFSObject::Type::file;
        object->remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("lastModifiedDateTime")].toString(),true);
        object->remote.md5Hash= o[QStringLiteral("file")].toObject()["hashes"].toObject()[QStringLiteral("sha1Hash")].toString();

        this->filelist_populateChanges(*object);

        file.close();
        delete(req);
        return true;

    }




}



bool MSOneDrive::remote_file_update(MSFSObject *object){

    return this->remote_file_insert(object);
}



//bool MSOneDrive::remote_file_generateIDs(int count){
//Q_UNUSED(count);
//    return true;
//}

bool MSOneDrive::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    if(object->path == QStringLiteral("/")){

        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root/children"));

    }
    else{

        QString rpath = object->path.left(object->path.length()-1);
        //req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:/"+object->path+":/children");
        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root:")+ QString(QUrl::toPercentEncoding(rpath)) +QStringLiteral(":/children"));
    }

    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make folder metadata in json representation
    QJsonObject metaJson;

    metaJson.insert(QStringLiteral("name"), object->fileName);
    metaJson.insert(QStringLiteral("folder"),QJsonObject());

//    QString hhh = QString::fromUtf8(QJsonDocument(metaJson).toJson());


    metaData.append(QJsonDocument(metaJson).toJson());

//    QTextCodec *codec = QTextCodec::codecForName( "UTF-16" );
//    metaData = codec->fromUnicode(metaData);

//    req->post(metaData);
    req->setInputDataStream(metaData);
    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    QString content=req->readReplyText();


    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject o = json.object();

    object->remote.fileSize=  o[QStringLiteral("size")].toInt();
    object->remote.data=o;
    object->remote.exist=true;
    object->isDocFormat=false;

    object->state=MSFSObject::ObjectState::Sync;

    object->remote.objectType=MSRemoteFSObject::Type::file;
    object->remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("lastModifiedDateTime")].toString(),true);
    object->remote.md5Hash= o[QStringLiteral("file")].toObject()[QStringLiteral("hashes")].toObject()[QStringLiteral("sha1Hash")].toString();

    this->filelist_populateChanges(*object);

    return true;


}

void MSOneDrive::remote_file_makeFolder(MSFSObject *object, const QString &parentID){

Q_UNUSED(object);
Q_UNUSED(parentID);

}

bool MSOneDrive::remote_file_trash(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    if(object->path == QStringLiteral("/")){

        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root:/")+QString(QUrl::toPercentEncoding(object->fileName,excludeChars)));

    }
    else{

        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root:")+QString(QUrl::toPercentEncoding(object->path+object->fileName,excludeChars)));
    }

    req->setMethod(QStringLiteral("delete"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);

//    req->deleteResource();
    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }


    if(((int)req->replyError == 204)||(int)req->replyError == 0){

        delete(req);
        return true;
    }
    else{

        delete(req);
        return false;
    }

}



bool MSOneDrive::remote_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    QList<QString> dirs=path.split(QStringLiteral("/"));
    QString currPath="";

    for(int i=1;i<dirs.size();i++){

        QHash<QString,MSFSObject>::iterator f=this->syncFileList.find(currPath+QStringLiteral("/")+dirs[i]);

        if(f != this->syncFileList.end()){

            currPath=f.key();

            if(f.value().remote.exist){
                continue;
            }

            if(this->filelist_FSObjectHasParent(f.value())){


                this->remote_file_makeFolder(&f.value());

            }
            else{

                this->remote_file_makeFolder(&f.value());
            }
        }

    }

    return true;

}



void MSOneDrive::local_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir d;
    d.mkpath(path);
}



void MSOneDrive::local_removeFile(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir trash(this->workPath+QStringLiteral("/")+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+QStringLiteral("/")+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+QStringLiteral("/")+this->trashFileName+path;

    // create trashed folder structure if it's needed
    QFileInfo tfi(trashedPath);
    QDir tfs(tfi.absolutePath().replace(this->workPath,""));
    if(!tfs.exists()){
        tfs.mkdir(this->workPath + tfi.absolutePath().replace(this->workPath,QStringLiteral("")));
    }

    QFile f;
    f.setFileName(origPath);
    bool res=f.rename(trashedPath);

    if((!res)&&(f.exists())){// everwrite trashed file
        QFile ef(trashedPath);
        ef.remove();
        f.rename(trashedPath);
    }
}



void MSOneDrive::local_removeFolder(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir trash(this->workPath+QStringLiteral("/")+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+QStringLiteral("/")+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+QStringLiteral("/")+this->trashFileName+path;

    // create trashed folder structure if it's needed
    QFileInfo tfi(trashedPath);
    QDir tfs(tfi.absolutePath().replace(this->workPath,QStringLiteral("")));
    if(!tfs.exists()){
        tfs.mkdir(this->workPath + tfi.absolutePath().replace(this->workPath,""));
    }

    QDir f;
    f.setPath(origPath);
    bool res=f.rename(origPath,trashedPath);

    if((!res)&&(f.exists())){// everwrite trashed folder
        QDir ef(trashedPath);
        ef.removeRecursively();
        f.rename(origPath,trashedPath);
    }
}



bool MSOneDrive::auth(){


    connect(this,SIGNAL(oAuthCodeRecived(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));
    connect(this,SIGNAL(oAuthError(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));



    this->startListener(1973);

    qStdOut()<<"-------------------------------------" <<endl;
    qStdOut()<< tr("Please go to this URL and confirm application credentials\n") <<endl;


    qStdOut() << QStringLiteral("https://login.live.com/oauth20_authorize.srf?client_id=07bcebfb-0764-450a-b9ef-e839c592a418&scope=onedrive.readwrite offline_access&response_type=code&redirect_uri=http://localhost:1973")<<endl  ;



    QEventLoop loop;
    connect(this, SIGNAL(providerAuthComplete()), &loop, SLOT(quit()));
    loop.exec();


    if(!this->providerAuthStatus){
        qStdOut() << "Code was not received. Some browsers handle redirect incorrectly. If it this case please copy a value of \"code\" parameter to the terminal and press enter "<< endl;
        QTextStream s(stdin);
        QString code =s.readLine();
        this->onAuthFinished(code,this);

    }

    return true;

}

void MSOneDrive::saveTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << QStringLiteral("{\"refresh_token\" : \"")+this->token+QStringLiteral("\"}");
    key.close();
}

bool MSOneDrive::loadTokenFile(const QString &path){

    QFile key(path+QStringLiteral("/")+this->tokenFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << QStringLiteral("Access key missing or corrupt. Start CloudCross with -a option for obtained private key.")<<endl   ;
        return false;
    }

    QTextStream instream(&key);
    QString line;
    while(!instream.atEnd()){

        line+=instream.readLine();
    }

    QJsonDocument json = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject job = json.object();
    QString v=job[QStringLiteral("refresh_token")].toString();

    this->token=v;

    key.close();
    return true;
}

void MSOneDrive::loadStateFile(){

    QFile key(this->credentialsPath+QStringLiteral("/")+this->stateFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << QStringLiteral("Previous state file not found. Start in stateless mode.")<<endl  ;
        return;
    }

    QTextStream instream(&key);
    QString line;
    while(!instream.atEnd()){

        line+=instream.readLine();
    }

    QJsonDocument json = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject job = json.object();

    this->lastSyncTime=QJsonValue(job[QStringLiteral("last_sync")].toObject()[QStringLiteral("sec")]).toVariant().toULongLong();

    key.close();
    return;

}

void MSOneDrive::saveStateFile(){

    QJsonDocument state;
    QJsonObject jso;
    jso.insert(QStringLiteral("change_stamp"),QStringLiteral("0"));

    QJsonObject jts;
    jts.insert(QStringLiteral("nsec"),QStringLiteral("0"));
    jts.insert(QStringLiteral("sec"),QString::number(QDateTime( QDateTime::currentDateTime()).toMSecsSinceEpoch()));

    jso.insert(QStringLiteral("last_sync"),jts);
    state.setObject(jso);

    QFile key(this->credentialsPath+QStringLiteral("/")+this->stateFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << state.toJson();
    key.close();
}

bool MSOneDrive::refreshToken(){

//    this->access_token=this->token;
//    return true;

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://login.live.com/oauth20_token.srf"));
    req->setMethod(QStringLiteral("post"));

   // req->addQueryItem("redirect_uri",           "localhost");
    req->addQueryItem(QStringLiteral("refresh_token"),          this->token);
    req->addQueryItem(QStringLiteral("client_id"),              QStringLiteral("07bcebfb-0764-450a-b9ef-e839c592a418"));
    req->addQueryItem(QStringLiteral("client_secret"),          QStringLiteral("FFWd2Pc5jbqqmTaknxMEYQ5"));
    req->addQueryItem(QStringLiteral("grant_type"),          QStringLiteral("refresh_token"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content= req->replyText;//lastReply->readAll();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    this->access_token=job[QStringLiteral("access_token")].toString();

    delete(req);
    return true;
}

MSFSObject::ObjectState MSOneDrive::filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote){



    if((local.exist)&&(remote.exist)){ //exists both files

        if(local.md5Hash.toLower()==remote.md5Hash.toLower()){


                return MSFSObject::ObjectState::Sync;

        }
        else{

            // compare last modified date for local and remote
            if(local.modifiedDate==remote.modifiedDate){

                if(this->strategy==MSCloudProvider::SyncStrategy::PreferLocal){
                    return MSFSObject::ObjectState::ChangedLocal;
                }
                else{
                    return MSFSObject::ObjectState::ChangedRemote;
                }

            }
            else{

                if(local.modifiedDate > remote.modifiedDate){
                    return MSFSObject::ObjectState::ChangedLocal;
                }
                else{
                    return MSFSObject::ObjectState::ChangedRemote;
                }

            }
        }


    }


    if((local.exist)&&(!remote.exist)){ //exist only local file

        if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){
            return  MSFSObject::ObjectState::NewLocal;
        }
        else{
            return  MSFSObject::ObjectState::DeleteRemote;
        }
    }


    if((!local.exist)&&(remote.exist)){ //exist only remote file

        if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){
            return  MSFSObject::ObjectState::DeleteLocal;
        }
        else{
            return  MSFSObject::ObjectState::NewRemote;
        }
    }



return  MSFSObject::ObjectState::ErrorState;


}



void MSOneDrive::checkFolderStructures(){

    QHash<QString,MSFSObject>::iterator lf;

    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

        // create new folder structure on remote

        qStdOut()<<"Checking folder structure on remote"<<endl  ;

        QHash<QString,MSFSObject> localFolders=this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::folder);
        localFolders=this->filelist_getFSObjectsByState(localFolders,MSFSObject::ObjectState::NewLocal);

        lf=localFolders.begin();

        while(lf != localFolders.end()){

            this->remote_createDirectory(lf.key());

            lf++;
        }
    }
    else{

        // create new folder structure on local

        qStdOut()<<"Checking folder structure on local" <<endl ;

        QHash<QString,MSFSObject> remoteFolders=this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::folder);
        remoteFolders=this->filelist_getFSObjectsByState(remoteFolders,MSFSObject::ObjectState::NewRemote);

        lf=remoteFolders.begin();

        while(lf != remoteFolders.end()){

            this->local_createDirectory(this->workPath+lf.key());

            lf++;
        }

        // trash local folder
        QHash<QString,MSFSObject> trashFolders=this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::folder);
        trashFolders=this->filelist_getFSObjectsByState(trashFolders,MSFSObject::ObjectState::DeleteRemote);

        lf=trashFolders.begin();

        while(lf != trashFolders.end()){


            this->local_removeFolder(lf.key());

            lf++;
        }

    }

}



void MSOneDrive::doSync(QHash<QString, MSFSObject> fsObjectList){

    QHash<QString,MSFSObject>::iterator lf;

    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag(QStringLiteral("force"))){

        if(this->getOption("force")=="download"){

            qStdOut()<<QStringLiteral("Start downloading in force mode")<<endl  ;

            lf=fsObjectList.begin();

            for(;lf != fsObjectList.end();lf++){

                MSFSObject obj=lf.value();

                if((obj.state == MSFSObject::ObjectState::Sync)||
                   (obj.state == MSFSObject::ObjectState::NewRemote)||
                   (obj.state == MSFSObject::ObjectState::DeleteLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedRemote) ){


                    if(obj.remote.objectType == MSRemoteFSObject::Type::file){

                        qStdOut()<< QString(obj.path + obj.fileName + QStringLiteral(" Forced downloading."))<<endl  ;

                        this->remote_file_get(&obj);
                    }
                }

            }

        }
        else{
            if(this->getOption(QStringLiteral("force"))==QStringLiteral("upload")){

                qStdOut()<<QStringLiteral("Start uploading in force mode")<<endl  ;

                lf=fsObjectList.begin();

                for(;lf != fsObjectList.end();lf++){

                    MSFSObject obj=lf.value();

                    if((obj.state == MSFSObject::ObjectState::Sync)||
                       (obj.state == MSFSObject::ObjectState::NewLocal)||
                       (obj.state == MSFSObject::ObjectState::DeleteRemote)||
                       (obj.state == MSFSObject::ObjectState::ChangedLocal)||
                       (obj.state == MSFSObject::ObjectState::ChangedRemote) ){

                        if(obj.remote.exist){

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< QString(obj.path + obj.fileName + QStringLiteral(" Forced uploading."))<<endl  ;

                                this->remote_file_update(&obj);
                            }
                        }
                        else{

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< QString(obj.path + obj.fileName + QStringLiteral(" Forced uploading."))<<endl  ;

                                this->remote_file_insert(&obj);
                            }
                        }


                    }

                }
            }
            else{
                // error
            }
        }


        if(this->getFlag(QStringLiteral("dryRun"))){
            return;
        }

        // save state file

        this->saveStateFile();




            qStdOut()<<QStringLiteral("Syncronization end")<<endl  ;

            return;
    }



    // SYNC FILES AND FOLDERS

    qStdOut()<<QStringLiteral("Start syncronization")<<endl  ;

    lf=fsObjectList.begin();

    for(;lf != fsObjectList.end();lf++){

        MSFSObject obj=lf.value();

        if((obj.state == MSFSObject::ObjectState::Sync)){

            continue;
        }

        switch((int)(obj.state)){

            case MSFSObject::ObjectState::ChangedLocal:

                qStdOut()<< QString(obj.path + obj.fileName + QStringLiteral(" Changed local. Uploading."))<<endl  ;

                this->remote_file_update(&obj);

                break;

            case MSFSObject::ObjectState::NewLocal:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    qStdOut()<< QString(obj.path + obj.fileName + QStringLiteral(" New local. Uploading."))<<endl  ;

                    this->remote_file_insert(&obj);

                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading."))<<endl  ;

                        this->remote_file_insert(&obj);

                    }
                    else{

                        qStdOut()<< QString(obj.path +obj.fileName +QStringLiteral(" Delete remote. Delete local."))<<endl  ;

                        if((obj.local.objectType == MSLocalFSObject::Type::file)||(obj.remote.objectType == MSRemoteFSObject::Type::file)){
                            this->local_removeFile(obj.path+obj.fileName);
                        }
                        else{
                            this->local_removeFolder(obj.path+obj.fileName);
                        }


                    }
                }


                break;

            case MSFSObject::ObjectState::ChangedRemote:

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Changed remote. Downloading."))<<endl  ;

                this->remote_file_get(&obj);

                break;


            case MSFSObject::ObjectState::NewRemote:

                if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") )<<endl ;

                        this->remote_file_trash(&obj);

                    }
                    else{
                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") )<<endl ;

                        this->remote_file_get(&obj);
                    }


                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote."))<<endl  ;

                        this->remote_file_trash(&obj);
                    }
                    else{

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading."))<<endl  ;

                        this->remote_file_get(&obj);
                    }
                }

                break;


            case MSFSObject::ObjectState::DeleteLocal:

                if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") )<<endl ;

                    this->remote_file_get(&obj);

                    break;
                }

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote."))<<endl  ;

                this->remote_file_trash(&obj);

                break;

            case MSFSObject::ObjectState::DeleteRemote:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") )<<endl ;

                        this->remote_file_insert(&obj);
                    }
                    else{
                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete remote. Deleting local.") )<<endl ;

                        if((obj.local.objectType == MSLocalFSObject::Type::file)||(obj.remote.objectType == MSRemoteFSObject::Type::file)){
                            this->local_removeFile(obj.path+obj.fileName);
                        }
                        else{
                            this->local_removeFolder(obj.path+obj.fileName);
                        }

                    }
                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") )<<endl ;

                        this->remote_file_insert(&obj);

                    }
                    else{

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete remote. Deleting local.") )<<endl ;

                        if((obj.local.objectType == MSLocalFSObject::Type::file)||(obj.remote.objectType == MSRemoteFSObject::Type::file)){
                            this->local_removeFile(obj.path+obj.fileName);
                        }
                        else{
                            this->local_removeFolder(obj.path+obj.fileName);
                        }


                    }
                }


                break;


        }


    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    // save state file

    this->saveStateFile();




        qStdOut()<<QStringLiteral("Syncronization end")<<endl  ;

}



QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByState(MSFSObject::ObjectState state){

    QHash<QString,MSFSObject> out;

    QHash<QString,MSFSObject>::iterator i=this->syncFileList.begin();

    while(i != this->syncFileList.end()){

        if(i.value().state == state){
            out.insert(i.key(),i.value());
        }

        i++;
    }

    return out;
}



QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByState( QHash<QString, MSFSObject> fsObjectList, MSFSObject::ObjectState state){

    QHash<QString,MSFSObject> out;

    QHash<QString,MSFSObject>::iterator i=fsObjectList.begin();

    while(i != fsObjectList.end()){

        if(i.value().state == state){
            out.insert(i.key(),i.value());
        }

        i++;
    }

    return out;

}



QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type){


    QHash<QString,MSFSObject> out;

    QHash<QString,MSFSObject>::iterator i=this->syncFileList.begin();

    while(i != this->syncFileList.end()){

        if(i.value().local.objectType == type){
            out.insert(i.key(),i.value());
        }

        i++;
    }

    return out;

}

QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type){

    QHash<QString,MSFSObject> out;

    QHash<QString,MSFSObject>::iterator i=this->syncFileList.begin();

    while(i != this->syncFileList.end()){

        if(i.value().remote.objectType == type){
            out.insert(i.key(),i.value());
        }

        i++;
    }

    return out;

}



bool MSOneDrive::filelist_FSObjectHasParent(const MSFSObject &fsObject){

    if(fsObject.path==QStringLiteral("/")){
        return false;
    }
    else{
        return true;
    }

//    if(fsObject.path.count(QStringLiteral("/"))>=1){
//        return true;
//    }
//    else{
//        return false;
//    }


}



MSFSObject MSOneDrive::filelist_getParentFSObject(const MSFSObject &fsObject){

    QString parentPath;

    if((fsObject.local.objectType==MSLocalFSObject::Type::file) || (fsObject.remote.objectType==MSRemoteFSObject::Type::file)){
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf("/"));
    }
    else{
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf("/"));
    }

    if(parentPath==QStringLiteral("")){
        parentPath=QStringLiteral("/");
    }

    QHash<QString,MSFSObject>::iterator parent=this->syncFileList.find(parentPath);

    if(parent != this->syncFileList.end()){
        return parent.value();
    }
    else{
        return MSFSObject();
    }

}



void MSOneDrive::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}

bool MSOneDrive::testReplyBodyForError(const QString &body){

    if(body.contains(QStringLiteral("\"error\":"))){

        return false;

    }
    else{
        return true;
    }


}

QString MSOneDrive::getReplyErrorString(const QString &body){

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job[QStringLiteral("error")].toObject()[QStringLiteral("message")]);

    return e.toString();
}

bool MSOneDrive::readRemote(const QString &rootPath){



    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    if(rootPath != ""){
        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root:")+QString(QUrl::toPercentEncoding(rootPath))+QStringLiteral(":/children"));
    }
    else{
        req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/root")+QString(QUrl::toPercentEncoding(rootPath))+QStringLiteral("/children"));

    }
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);


    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }


    QString content= req->replyText;
    delete(req); // Merge pull request #36 from trufanov-nok/patch-2

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QJsonArray items=job[QStringLiteral("value")].toArray();

    bool hasMore=false;



    do{

        if(job[QStringLiteral("@odata.nextLink")].toString() != QStringLiteral("")){
            hasMore=true;
        }
        else{
            hasMore=false;
        }

        int sz=items.size();

        for(int i=0; i<sz; i++){

            MSFSObject fsObject;
            QJsonObject o=items[i].toObject();

            fsObject.fileName= o[QStringLiteral("name")].toString();

            if(rootPath ==QStringLiteral("")){
                fsObject.path= "/";
            }
            else{
                fsObject.path = rootPath+QStringLiteral("/");
            }


            fsObject.remote.fileSize=  o[QStringLiteral("size")].toInt();
            fsObject.remote.data=o;
            fsObject.remote.exist=true;
            fsObject.isDocFormat=false;

            fsObject.state=MSFSObject::ObjectState::NewRemote;

            if( this->isFile(o) ){

                fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("lastModifiedDateTime")].toString(),true);
                fsObject.remote.md5Hash= o[QStringLiteral("file")].toObject()[QStringLiteral("hashes")].toObject()[QStringLiteral("sha1Hash")].toString();

            }

            if( this->isFolder(items[i]) ){

                fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("lastModifiedDateTime")].toString(),true);

                this->readRemote(fsObject.path+fsObject.fileName);
            }

            //--patch by trufanov-nok

            if(! this->filterServiceFileNames(fsObject.path+fsObject.fileName)){// skip service files and dirs

                continue;
            }


            if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

                if( this->filterIncludeFileNames(fsObject.path+fsObject.fileName)){

                    continue;
                }
            }
            else{// use exclude by default

                if(this->excludeList != QStringLiteral("")){
                    if(! this->filterExcludeFileNames(fsObject.path+fsObject.fileName)){

                        continue;
                    }
                }
            }

            this->syncFileList.insert(fsObject.path+fsObject.fileName, fsObject);

        }

        if(hasMore){

            MSHttpRequest* mrq=new MSHttpRequest(this->proxyServer);

            QString nl=job[QStringLiteral("@odata.nextLink")].toString();

            mrq->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);

//            mrq->raw_exec(nl);
            mrq->setRequestUrl(nl);
            mrq->setMethod(QStringLiteral("get"));
            mrq->exec();

            if(!mrq->replyOK()){
                mrq->printReplyError();
                delete(mrq);
                return false;
            }


            content= mrq->replyText;


            json = QJsonDocument::fromJson(content.toUtf8());

            job = json.object();
            items=job[QStringLiteral("value")].toArray();

        }

    }while(hasMore);


    return true;
}

bool MSOneDrive::_readRemote(const QString &rootPath){

    return this->readRemote(rootPath);

}

bool MSOneDrive::readLocal(const QString &path){

    QDir dir(path);
    QDir::Filters entryInfoList_flags=QDir::Files|QDir::Dirs |QDir::NoDotAndDotDot;

    if(! this->getFlag(QStringLiteral("noHidden"))){// if show hidden
        entryInfoList_flags= entryInfoList_flags | QDir::System | QDir::Hidden;
    }

        QFileInfoList files = dir.entryInfoList(entryInfoList_flags);

        foreach(const QFileInfo &fi, files){

            QString Path = fi.absoluteFilePath();
            QString relPath=fi.absoluteFilePath().replace(this->workPath,"");

            if(! this->filterServiceFileNames(relPath)){// skip service files and dirs
                continue;
            }


            if(fi.isDir()){

                readLocal(Path);
            }

            if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

                if( this->filterIncludeFileNames(relPath)){

                    continue;
                }
            }
            else{// use exclude by default

                if(this->excludeList != QStringLiteral("")){
                    if(! this->filterExcludeFileNames(relPath)){

                    continue;
                    }
                }
            }



            QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



            if(i!=this->syncFileList.end()){// if object exists in OneDrive

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
                fsObject->local.exist=true;
                fsObject->getLocalMimeType(this->workPath);

                if(fi.isDir()){
                    fsObject->local.objectType=MSLocalFSObject::Type::folder;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);
                }
                else{

                    fsObject->local.objectType=MSLocalFSObject::Type::file;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);

                }


                fsObject->isDocFormat=false;


                fsObject->state=this->filelist_defineObjectState(fsObject->local,fsObject->remote);

            }
            else{

                MSFSObject fsObject;

                fsObject.state=MSFSObject::ObjectState::NewLocal;

                if(relPath.lastIndexOf(QStringLiteral("/"))==0){
                    fsObject.path=QStringLiteral("/");
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf(QStringLiteral("/")))+QStringLiteral("/");
                }

                fsObject.fileName=fi.fileName();
                fsObject.getLocalMimeType(this->workPath);

                fsObject.local.fileSize=  fi.size();
                fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
                fsObject.local.exist=true;

                if(fi.isDir()){
                    fsObject.local.objectType=MSLocalFSObject::Type::folder;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);
                }
                else{

                    fsObject.local.objectType=MSLocalFSObject::Type::file;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);

                }

                fsObject.state=this->filelist_defineObjectState(fsObject.local,fsObject.remote);

                fsObject.isDocFormat=false;


                this->syncFileList.insert(relPath,fsObject);

            }

        }

        return true;



}



bool MSOneDrive::readLocalSingle(const QString &path){

    QFileInfo fi(path);


            QString Path = fi.absoluteFilePath();
            QString relPath=fi.absoluteFilePath().replace(this->workPath,"");

            if(! this->filterServiceFileNames(relPath)){// skip service files and dirs
                return false;
            }


            if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

                if( this->filterIncludeFileNames(relPath)){

                    return false;
                }
            }
            else{// use exclude by default

                if(this->excludeList != QStringLiteral("")){
                    if(! this->filterExcludeFileNames(relPath)){

                        return false;
                    }
                }
            }



            QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



            if(i!=this->syncFileList.end()){// if object exists in OneDrive

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
                fsObject->local.exist=true;
                fsObject->getLocalMimeType(this->workPath);

                if(fi.isDir()){
                    fsObject->local.objectType=MSLocalFSObject::Type::folder;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);
                }
                else{

                    fsObject->local.objectType=MSLocalFSObject::Type::file;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);

                }


                fsObject->isDocFormat=false;


                fsObject->state=this->filelist_defineObjectState(fsObject->local,fsObject->remote);

            }
            else{

                MSFSObject fsObject;

                fsObject.state=MSFSObject::ObjectState::NewLocal;

                if(relPath.lastIndexOf(QStringLiteral("/"))==0){
                    fsObject.path=QStringLiteral("/");
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf(QStringLiteral("/")))+QStringLiteral("/");
                }

                fsObject.fileName=fi.fileName();
                fsObject.getLocalMimeType(this->workPath);

                fsObject.local.fileSize=  fi.size();
                fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
                fsObject.local.exist=true;

                if(fi.isDir()){
                    fsObject.local.objectType=MSLocalFSObject::Type::folder;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);
                }
                else{

                    fsObject.local.objectType=MSLocalFSObject::Type::file;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true);

                }

                fsObject.state=this->filelist_defineObjectState(fsObject.local,fsObject.remote);

                fsObject.isDocFormat=false;


                this->syncFileList.insert(relPath,fsObject);

            }



        return true;



}




bool MSOneDrive::isFolder(const QJsonValue &remoteObject){

    QJsonValue s=remoteObject.toObject()[QStringLiteral("folder")];

    if(s != QJsonValue()){
        return true;
    }

    return false;
}



bool MSOneDrive::isFile(const QJsonValue &remoteObject){

    QJsonValue s=remoteObject.toObject()[QStringLiteral("file")];

    if(s != QJsonValue()){
        return true;
    }

    return false;

}

bool MSOneDrive::createSyncFileList(){

    if(this->getFlag(QStringLiteral("useInclude"))){
        QFile key(this->workPath+QStringLiteral("/.include"));

        if(key.open(QIODevice::ReadOnly)){

            QTextStream instream(&key);
            QString line;
            while(!instream.atEnd()){

                line=instream.readLine();
                if(line.isEmpty()){
                    continue;
                }
                if(instream.pos() == 9 && line == QStringLiteral("wildcard")){
                    this->options.insert(QStringLiteral("filter-type"), QStringLiteral("wildcard"));
                    continue;
                }
                else if(instream.pos() == 7 && line == QStringLiteral("regexp")){
                    this->options.insert(QStringLiteral("filter-type"), QStringLiteral("regexp"));
                    continue;
                }
                this->includeList=this->includeList+line+"|";
            }
            this->includeList=this->includeList.left(this->includeList.size()-1);

            QRegExp regex2(this->includeList);
            if(this->getOption(QStringLiteral("filter-type")) == QStringLiteral("regexp"))
                regex2.setPatternSyntax(QRegExp::RegExp);
            else
                regex2.setPatternSyntax(QRegExp::Wildcard);
            if(!regex2.isValid()){
                qStdOut() << QStringLiteral("Include filelist contains errors. Program will be terminated.")<<endl;
                return false;
            }
        }
    }
    else{
        QFile key(this->workPath+QStringLiteral("/.exclude"));

        if(key.open(QIODevice::ReadOnly)){

            QTextStream instream(&key);
            QString line;
            while(!instream.atEnd()){

                line=instream.readLine();
                if(instream.pos() == 9 && line == QStringLiteral("wildcard")){
                    this->options.insert(QStringLiteral("filter-type"), QStringLiteral("wildcard"));
                    continue;
                }
                else if(instream.pos() == 7 && line == QStringLiteral("regexp")){
                    this->options.insert(QStringLiteral("filter-type"), QStringLiteral("regexp"));
                    continue;
                }
                if(line.isEmpty()){
                    continue;
                }
                this->excludeList=this->excludeList+line+"|";
            }
            this->excludeList=this->excludeList.left(this->excludeList.size()-1);

            QRegExp regex2(this->excludeList);
            if(this->getOption(QStringLiteral("filter-type")) == QStringLiteral("regexp"))
                regex2.setPatternSyntax(QRegExp::RegExp);
            else
                regex2.setPatternSyntax(QRegExp::Wildcard);
            if(!regex2.isValid()){
                qStdOut()<<QStringLiteral("Exclude filelist contains errors. Program will be terminated.")<<endl;
                return false;
            }
        }
    }

    qStdOut()<< QStringLiteral("Reading remote files")<<endl ;


    if(!this->readRemote(QStringLiteral(""))){// top level files and folders
        qStdOut()<<QStringLiteral("Error occured on reading remote files")<<endl  ;
        return false;

    }

    qStdOut()<<QStringLiteral("Reading local files and folders")<<endl  ;

    if(!this->readLocal(this->workPath)){
        qStdOut()<<QStringLiteral("Error occured on local files and folders")<<endl  ;
        return false;

    }

//this->remote_file_get(&(this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::file).begin().value()));//&(this->syncFileList.values()[1])
//this->remote_file_insert(&(this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::file).begin().value()));
//this->remote_file_update(&(this->syncFileList.values()[0]));
// this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
//    this->remote_file_trash(&(this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::file).begin().value()));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));


    // make separately lists of objects
    QList<QString> keys = this->syncFileList.uniqueKeys();

    if((keys.size()>3) && (this->getFlag(QStringLiteral("singleThread")) == false)){// split list to few parts

        this->threadsRunning = new QSemaphore(3);

        QThread* t1 = new QThread(this);
        QThread* t2 = new QThread();
        QThread* t3 = new QThread(this);

        MSSyncThread* thr1 = new MSSyncThread(nullptr,this);
        thr1->moveToThread(t1);
        connect(t1,SIGNAL(started()),thr1,SLOT(run()));
        connect(t1,SIGNAL(finished()),thr1,SLOT(deleteLater()));
        connect(thr1,SIGNAL(finished()),t1,SLOT(quit()));

        MSSyncThread* thr2 = new MSSyncThread(nullptr,this);
        thr2->moveToThread(t2);
        connect(t2,SIGNAL(started()),thr2,SLOT(run()));
        connect(t2,SIGNAL(finished()),thr2,SLOT(deleteLater()));
        connect(thr2,SIGNAL(finished()),t2,SLOT(quit()));

        MSSyncThread* thr3 = new MSSyncThread(nullptr,this);
        thr3->moveToThread(t3);
        connect(t3,SIGNAL(started()),thr3,SLOT(run()));
        connect(t3,SIGNAL(finished()),thr3,SLOT(deleteLater()));
        connect(thr3,SIGNAL(finished()),t3,SLOT(quit()));


        MSSyncThread* threads[3] = {thr1, thr2, thr3};
        int j = 0;
        for(int i = 0; i<keys.size(); i++ ){
            threads[j++]->threadSyncList.insert(keys[i],this->syncFileList.find(keys[i]).value());
            if (j == 3) j = 0;
        }

        this->checkFolderStructures();

        t1->start();
        t2->start();
        t3->start();
        t1->wait();
        t2->wait();
        t3->wait();

//        while (this->threadsRunning->available()<3) {

//        }


    }
    else{// sync as is

        this->checkFolderStructures();
        this->doSync(this->syncFileList);
    }

    return true;

}


// TODO: Need way how does it
bool MSOneDrive::directUpload(const QString &url, const QString &remotePath){
Q_UNUSED(remotePath);
Q_UNUSED(url);

    qStdOut() << "NOT INPLEMENTED YET"<<endl;
    return false;

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive/items/D7C91EBFD21F9BA0!510/children"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
    req->addHeader(QStringLiteral("Prefer"), QStringLiteral("respond-async"));


    QByteArray ba;

    ba="{ \
       \"@microsoft.graph.sourceUrl\": \"https://mastersoft24.ru/img/small-logo.png\", \
       \"name\": \"halo-screenshot.jpg\" ,\
        \"file\": { } \
     }";

    req->post(ba);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        //return "false";
    }



    QString content= req->replyText;//lastReply->readAll();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    return true;
}



QString MSOneDrive::getInfo(){

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.onedrive.com/v1.0/drive"));
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return QStringLiteral("false");
    }



    QString content= req->replyText;//lastReply->readAll();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    delete(req);

    QJsonObject out;
    out[QStringLiteral("account")]=job[QStringLiteral("owner")].toObject()[QStringLiteral("user")].toObject()[QStringLiteral("displayName")].toString();
    out[QStringLiteral("total")]= QString::number( (uint64_t)job[QStringLiteral("quota")].toObject()[QStringLiteral("total")].toDouble());
    out[QStringLiteral("usage")]= QString::number( (uint64_t)job[QStringLiteral("quota")].toObject()[QStringLiteral("used")].toDouble());

    return QString( QJsonDocument(out).toJson());

}



bool MSOneDrive::onAuthFinished(const QString &html, MSCloudProvider *provider){

    Q_UNUSED(provider);

        MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

        req->setRequestUrl(QLatin1String("https://login.live.com/oauth20_token.srf"));
        req->setMethod(QStringLiteral("post"));

        req->addQueryItem(QStringLiteral("redirect_uri"),           QStringLiteral("http://localhost:1973"));
        req->addQueryItem(QStringLiteral("grant_type"),          QStringLiteral("authorization_code"));
        req->addQueryItem(QStringLiteral("client_id"),              QStringLiteral("07bcebfb-0764-450a-b9ef-e839c592a418"));
        req->addQueryItem(QStringLiteral("client_secret"),              QStringLiteral("FFWd2Pc5jbqqmTaknxMEYQ5"));
        req->addQueryItem(QStringLiteral("code"),                  html);

        req->exec();


        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }



        QString content= req->replyText;//lastReply->readAll();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();
        QString v=job[QStringLiteral("refresh_token")].toString();

        if(v!=""){

            this->token=v;
            qStdOut() << QStringLiteral("Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize.")<<endl ;
            this->providerAuthStatus=true;
            emit providerAuthComplete();
            return true;
        }
        else{
            this->providerAuthStatus=false;
            emit providerAuthComplete();
            return false;
        }

}


bool MSOneDrive::remote_file_empty_trash(){


    qStdOut() << "This option does not supported by this provider" << endl;
    return false;
}
