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

MSOneDrive::MSOneDrive():
    MSCloudProvider()

{
    this->providerName=     "OneDrive";
    this->tokenFileName=    ".ondr";
    this->stateFileName=    ".ondr_state";
    this->trashFileName=    ".trash_ondr";
}

bool MSOneDrive::remote_file_get(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return true;
    }

    if(object->remote.data["id"].toString()=="")return false; //remove me

    QString filePath=this->workPath+object->path+object->fileName;

    MSRequest *req = new MSRequest(this->proxyServer);
    req->setMethod("get");

    req->addHeader("Authorization","Bearer "+this->access_token);


    req->download(object->remote.data["@content.downloadUrl"].toString(),this->workPath+object->path+object->fileName);

    QString c=req->readReplyText();

    if((int)req->replyError == 0){

        utimbuf tb;

        QString dd=object->remote.data["fileSystemInfo"].toObject()["lastModifiedDateTime"].toString();
        tb.actime=(this->toMilliseconds(dd,true))/1000;;
        tb.modtime=(this->toMilliseconds(dd,true))/1000;;

        utime(filePath.toStdString().c_str(),&tb);

        delete(req);
        return true;

    }

    return false;

//    QString id=object->remote.data["id"].toString();

//    //if(id=="")return false; //remove me

//    MSRequest *req = new MSRequest(this->proxyServer);

//    req->setRequestUrl("https://api.onedrive.com/v1.0/drive/items/"+QString(id)+"/content");
//    req->setMethod("get");

//    req->addHeader("Authorization","Bearer "+this->access_token);

//    req->exec();



//    QString filePath=this->workPath+object->path+object->fileName;


//    if(this->testReplyBodyForError(req->readReplyText())){

//        if(object->remote.objectType==MSRemoteFSObject::Type::file){

//            this->local_writeFileContent(filePath,req);
//            // set remote "change time" for local file

//            utimbuf tb;

//            QString dd=object->remote.data["fileSystemInfo"].toObject()["lastModifiedDateTime"].toString();
//            tb.actime=(this->toMilliseconds(dd,true))/1000;;
//            tb.modtime=(this->toMilliseconds(dd,true))/1000;;

//            utime(filePath.toStdString().c_str(),&tb);
//        }
//    }
//    else{

//        if(! this->getReplyErrorString(req->readReplyText()).contains( "path/not_file/")){
//            qStdOut() << "Service error. "<< this->getReplyErrorString(req->readReplyText());
//            delete(req);
//            return false;
//        }
//    }


//    delete(req);
//    return true;

}

bool MSOneDrive::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return true;
    }

    if(this->getFlag("dryRun")){
        return true;
    }


    // Create an upload session ===========

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:"+QString(object->path+object->fileName)+":/upload.createSession");
    req->setMethod("post");

    req->addHeader("Authorization","Bearer "+this->access_token);

//    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));
//    req->addQueryItem("path",                           object->path+object->fileName);
//    req->addQueryItem("overwrite",                           "true");

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString uploadUrl=job["uploadUrl"].toString();

    QString filePath=this->workPath+object->path+object->fileName;

    if(uploadUrl == ""){

        delete(req);
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
        return false;
    }

    quint64 cursorPosition=0;



    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    quint64 passCount=(quint64)((double)fSize/(double)ONEDRIVE_MAX_FILESIZE);// count of 59MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }

    delete(req);

    if(passCount==0){// onepass and finalize uploading

        req = new MSRequest(this->proxyServer);
        req->setRequestUrl(uploadUrl);

        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Length",                    QString::number(fSize).toLocal8Bit());

        if(fSize >0){
            req->addHeader("Content-Range",                     "bytes 0-"+QString::number(fSize-1).toLocal8Bit()+"/"+QString::number(fSize).toLocal8Bit());
        }
        else{
            qStdOut()<< "    OneDrive does not support zero-length files. Uploading skiped."<< endl;
            delete(req);
            return false;
        }

        QByteArray* ba=new QByteArray();

        file.seek(cursorPosition);
        ba->append(file.read(ONEDRIVE_MAX_FILESIZE));

        req->put(*ba);

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject o = json.object();

        object->remote.fileSize=  o["size"].toInt();
        object->remote.data=o;
        object->remote.exist=true;
        object->isDocFormat=false;

        object->state=MSFSObject::ObjectState::Sync;

        object->remote.objectType=MSRemoteFSObject::Type::file;
        object->remote.modifiedDate=this->toMilliseconds(o["lastModifiedDateTime"].toString(),true);
        object->remote.md5Hash= o["file"].toObject()["hashes"].toObject()["sha1Hash"].toString();

        this->filelist_populateChanges(*object);

        return true;

    }
    else{ // multipass uploading

        do{

            req = new MSRequest(this->proxyServer);
            req->setRequestUrl(uploadUrl);

            //set upload block size

            quint64 blsz=0;

            if(cursorPosition == 0){

                blsz=ONEDRIVE_MAX_FILESIZE;
            }
            else{

                if(fSize - cursorPosition < ONEDRIVE_MAX_FILESIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=ONEDRIVE_MAX_FILESIZE;
                }

            }

            //----------------------

            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Length",                    QString::number(blsz).toLocal8Bit());

            QString t= "bytes "+QString::number(cursorPosition).toLocal8Bit()+"-"+QString::number(cursorPosition+blsz-1).toLocal8Bit()+"/"+QString::number(fSize).toLocal8Bit();

            req->addHeader("Content-Range",                     "bytes "+QString::number(cursorPosition).toLocal8Bit()+"-"+QString::number(cursorPosition+blsz-1).toLocal8Bit()+"/"+QString::number(fSize).toLocal8Bit());

            QByteArray* ba=new QByteArray();

            file.seek(cursorPosition);
            ba->append(file.read(ONEDRIVE_MAX_FILESIZE));

            req->put(*ba);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            QString content=req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject o = json.object();

            QString cp=o["nextExpectedRanges"].toArray()[0].toString().split("-")[0];

            if(((int)req->replyError == 200)||((int)req->replyError == 201) || (cp == "") ){

                break;
            }

            cursorPosition=cp.toLong();

            delete(req);
            delete(ba);

        }while(true);

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject o = json.object();

        object->remote.fileSize=  o["size"].toInt();
        object->remote.data=o;
        object->remote.exist=true;
        object->isDocFormat=false;

        object->state=MSFSObject::ObjectState::Sync;

        object->remote.objectType=MSRemoteFSObject::Type::file;
        object->remote.modifiedDate=this->toMilliseconds(o["lastModifiedDateTime"].toString(),true);
        object->remote.md5Hash= o["file"].toObject()["hashes"].toObject()["sha1Hash"].toString();

        this->filelist_populateChanges(*object);

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

    if(this->getFlag("dryRun")){
        return true;
    }

    MSRequest *req = new MSRequest(this->proxyServer);

    if(object->path == "/"){

        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root/children");

    }
    else{

        //req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:/"+object->path+":/children");
        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:"+object->path+":/children");
    }

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make folder metadata in json representation
    QJsonObject metaJson;
    metaJson.insert("name", object->fileName);
    metaJson.insert("folder",QJsonObject());


    metaData.append((QJsonDocument(metaJson).toJson()));

    req->post(metaData);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject o = json.object();

    object->remote.fileSize=  o["size"].toInt();
    object->remote.data=o;
    object->remote.exist=true;
    object->isDocFormat=false;

    object->state=MSFSObject::ObjectState::Sync;

    object->remote.objectType=MSRemoteFSObject::Type::file;
    object->remote.modifiedDate=this->toMilliseconds(o["lastModifiedDateTime"].toString(),true);
    object->remote.md5Hash= o["file"].toObject()["hashes"].toObject()["sha1Hash"].toString();

    this->filelist_populateChanges(*object);

    return true;


}

void MSOneDrive::remote_file_makeFolder(MSFSObject *object, const QString &parentID){

Q_UNUSED(object);
Q_UNUSED(parentID);

}

bool MSOneDrive::remote_file_trash(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return true;
    }

    MSRequest *req = new MSRequest(this->proxyServer);

    if(object->path == "/"){

        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:/"+QUrl::toPercentEncoding(object->fileName));

    }
    else{

        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:"+QUrl::toPercentEncoding(object->path+object->fileName));
    }

    req->addHeader("Authorization",                     "Bearer "+this->access_token);

    req->deleteResource();


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

    if(this->getFlag("dryRun")){
        return true;
    }


    QList<QString> dirs=path.split("/");
    QString currPath="";

    for(int i=1;i<dirs.size();i++){

        QHash<QString,MSFSObject>::iterator f=this->syncFileList.find(currPath+"/"+dirs[i]);

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

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);
}



void MSOneDrive::local_removeFile(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir trash(this->workPath+"/"+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+"/"+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+"/"+this->trashFileName+path;

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

    if(this->getFlag("dryRun")){
        return;
    }

    QDir trash(this->workPath+"/"+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+"/"+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+"/"+this->trashFileName+path;

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

    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<< tr("Please go to this URL and confirm application credentials\n")<<endl;


    qStdOut() << "https://login.live.com/oauth20_authorize.srf?client_id=07bcebfb-0764-450a-b9ef-e839c592a418&scope=onedrive.readwrite offline_access&response_type=code&redirect_uri=http://localhost:1973" <<endl;



    QEventLoop loop;
    connect(this, SIGNAL(providerAuthComplete()), &loop, SLOT(quit()));
    loop.exec();




return true;

}

void MSOneDrive::saveTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"refresh_token\" : \""+this->token+"\"}";
    key.close();
}

bool MSOneDrive::loadTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << "Access key missing or corrupt. Start CloudCross with -a option for obtained private key."  <<endl;
        return false;
    }

    QTextStream instream(&key);
    QString line;
    while(!instream.atEnd()){

        line+=instream.readLine();
    }

    QJsonDocument json = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject job = json.object();
    QString v=job["refresh_token"].toString();

    this->token=v;

    key.close();
    return true;
}

void MSOneDrive::loadStateFile(){

    QFile key(this->credentialsPath+"/"+this->stateFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << "Previous state file not found. Start in stateless mode."<<endl ;
        return;
    }

    QTextStream instream(&key);
    QString line;
    while(!instream.atEnd()){

        line+=instream.readLine();
    }

    QJsonDocument json = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject job = json.object();

    this->lastSyncTime=QJsonValue(job["last_sync"].toObject()["sec"]).toVariant().toULongLong();

    key.close();
    return;

}

void MSOneDrive::saveStateFile(){

    QJsonDocument state;
    QJsonObject jso;
    jso.insert("change_stamp",QString("0"));

    QJsonObject jts;
    jts.insert("nsec",QString("0"));
    jts.insert("sec",QString::number(QDateTime( QDateTime::currentDateTime()).toMSecsSinceEpoch()));

    jso.insert("last_sync",jts);
    state.setObject(jso);

    QFile key(this->credentialsPath+"/"+this->stateFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << state.toJson();
    key.close();
}

bool MSOneDrive::refreshToken(){

//    this->access_token=this->token;
//    return true;

    MSRequest* req=new MSRequest(this->proxyServer);

    req->setRequestUrl("https://login.live.com/oauth20_token.srf");
    req->setMethod("post");

   // req->addQueryItem("redirect_uri",           "localhost");
    req->addQueryItem("refresh_token",          this->token);
    req->addQueryItem("client_id",              "07bcebfb-0764-450a-b9ef-e839c592a418");
    req->addQueryItem("client_secret",          "FFWd2Pc5jbqqmTaknxMEYQ5");
    req->addQueryItem("grant_type",          "refresh_token");

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content= req->replyText;//lastReply->readAll();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    this->access_token=job["access_token"].toString();

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



void MSOneDrive::doSync(){

    QHash<QString,MSFSObject>::iterator lf;

    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

        // create new folder structure on remote

        qStdOut()<<"Checking folder structure on remote" <<endl;

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

        qStdOut()<<"Checking folder structure on local" <<endl;

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




    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag("force")){

        if(this->getOption("force")=="download"){

            qStdOut()<<"Start downloading in force mode" <<endl;

            lf=this->syncFileList.begin();

            for(;lf != this->syncFileList.end();lf++){

                MSFSObject obj=lf.value();

                if((obj.state == MSFSObject::ObjectState::Sync)||
                   (obj.state == MSFSObject::ObjectState::NewRemote)||
                   (obj.state == MSFSObject::ObjectState::DeleteLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedRemote) ){


                    if(obj.remote.objectType == MSRemoteFSObject::Type::file){

                        qStdOut()<< obj.path<<obj.fileName <<" Forced downloading." <<endl;

                        this->remote_file_get(&obj);
                    }
                }

            }

        }
        else{
            if(this->getOption("force")=="upload"){

                qStdOut()<<"Start uploading in force mode" <<endl;

                lf=this->syncFileList.begin();

                for(;lf != this->syncFileList.end();lf++){

                    MSFSObject obj=lf.value();

                    if((obj.state == MSFSObject::ObjectState::Sync)||
                       (obj.state == MSFSObject::ObjectState::NewLocal)||
                       (obj.state == MSFSObject::ObjectState::DeleteRemote)||
                       (obj.state == MSFSObject::ObjectState::ChangedLocal)||
                       (obj.state == MSFSObject::ObjectState::ChangedRemote) ){

                        if(obj.remote.exist){

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< obj.path<<obj.fileName <<" Forced uploading." <<endl;

                                this->remote_file_update(&obj);
                            }
                        }
                        else{

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< obj.path<<obj.fileName <<" Forced uploading." <<endl;

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


        if(this->getFlag("dryRun")){
            return;
        }

        // save state file

        this->saveStateFile();




            qStdOut()<<"Syncronization end" <<endl;

            return;
    }



    // SYNC FILES AND FOLDERS

    qStdOut()<<"Start syncronization" <<endl;

    lf=this->syncFileList.begin();

    for(;lf != this->syncFileList.end();lf++){

        MSFSObject obj=lf.value();

        if((obj.state == MSFSObject::ObjectState::Sync)){

            continue;
        }

        switch((int)(obj.state)){

            case MSFSObject::ObjectState::ChangedLocal:

                qStdOut()<< obj.path<<obj.fileName <<" Changed local. Uploading." <<endl;

                this->remote_file_update(&obj);

                break;

            case MSFSObject::ObjectState::NewLocal:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    qStdOut()<< obj.path<<obj.fileName <<" New local. Uploading." <<endl;

                    this->remote_file_insert(&obj);

                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< obj.path<<obj.fileName <<" New local. Uploading." <<endl;

                        this->remote_file_insert(&obj);

                    }
                    else{

                        qStdOut()<< obj.path<<obj.fileName <<" Delete remote. Delete local." <<endl;

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

                qStdOut()<< obj.path<<obj.fileName <<" Changed remote. Downloading." <<endl;

                this->remote_file_get(&obj);

                break;


            case MSFSObject::ObjectState::NewRemote:

                if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< obj.path<<obj.fileName <<" Delete local. Deleting remote." <<endl;

                        this->remote_file_trash(&obj);

                    }
                    else{
                        qStdOut()<< obj.path<<obj.fileName <<" New remote. Downloading." <<endl;

                        this->remote_file_get(&obj);
                    }


                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< obj.path<<obj.fileName <<" Delete local. Deleting remote." <<endl;

                        this->remote_file_trash(&obj);
                    }
                    else{

                        qStdOut()<< obj.path<<obj.fileName <<" New remote. Downloading." <<endl;

                        this->remote_file_get(&obj);
                    }
                }

                break;


            case MSFSObject::ObjectState::DeleteLocal:

                if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    qStdOut()<< obj.path<<obj.fileName <<" New remote. Downloading." <<endl;

                    this->remote_file_get(&obj);

                    break;
                }

                qStdOut()<< obj.fileName <<" Delete local. Deleting remote." <<endl;

                this->remote_file_trash(&obj);

                break;

            case MSFSObject::ObjectState::DeleteRemote:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< obj.path<<obj.fileName <<" New local. Uploading." <<endl;

                        this->remote_file_insert(&obj);
                    }
                    else{
                        qStdOut()<< obj.path<<obj.fileName <<" Delete remote. Deleting local." <<endl;

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

                        qStdOut()<< obj.path<<obj.fileName <<" New local. Uploading." <<endl;

                        this->remote_file_insert(&obj);

                    }
                    else{

                        qStdOut()<< obj.path<<obj.fileName <<" Delete remote. Deleting local." <<endl;

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

    if(this->getFlag("dryRun")){
        return;
    }

    // save state file

    this->saveStateFile();




        qStdOut()<<"Syncronization end" <<endl;

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

    if(fsObject.path=="/"){
        return false;
    }
    else{
        return true;
    }

    if(fsObject.path.count("/")>=1){
        return true;
    }
    else{
        return false;
    }


}



MSFSObject MSOneDrive::filelist_getParentFSObject(const MSFSObject &fsObject){

    QString parentPath;

    if((fsObject.local.objectType==MSLocalFSObject::Type::file) || (fsObject.remote.objectType==MSRemoteFSObject::Type::file)){
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf("/"));
    }
    else{
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf("/"));
    }

    if(parentPath==""){
        parentPath="/";
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

    if(body.contains("\"error\":")){

        return false;

    }
    else{
        return true;
    }


}

QString MSOneDrive::getReplyErrorString(const QString &body){

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job["error"].toObject()["message"]);

    return e.toString();
}

bool MSOneDrive::readRemote(const QString &rootPath){



    MSRequest* req=new MSRequest(this->proxyServer);

    if(rootPath != ""){
        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:"+rootPath+":/children");
    }
    else{
        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root"+rootPath+"/children");

    }
    req->setMethod("get");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);


    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }


    QString content= req->replyText;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QJsonArray items=job["value"].toArray();

    bool hasMore=false;



    do{

        if(job["@odata.nextLink"].toString() != ""){
            hasMore=true;
        }
        else{
            hasMore=false;
        }

        int sz=items.size();

        for(int i=0; i<sz; i++){

            MSFSObject fsObject;
            QJsonObject o=items[i].toObject();

            fsObject.fileName= o["name"].toString();

            if(rootPath ==""){
                fsObject.path= "/";
            }
            else{
                fsObject.path = rootPath+"/";
            }


            fsObject.remote.fileSize=  o["size"].toInt();
            fsObject.remote.data=o;
            fsObject.remote.exist=true;
            fsObject.isDocFormat=false;

            fsObject.state=MSFSObject::ObjectState::NewRemote;

            if( this->isFile(o) ){

                fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                fsObject.remote.modifiedDate=this->toMilliseconds(o["lastModifiedDateTime"].toString(),true);
                fsObject.remote.md5Hash= o["file"].toObject()["hashes"].toObject()["sha1Hash"].toString();

            }

            if( this->isFolder(items[i]) ){

                fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                fsObject.remote.modifiedDate=this->toMilliseconds(o["lastModifiedDateTime"].toString(),true);

                this->readRemote(fsObject.path+fsObject.fileName);
            }



            this->syncFileList.insert(fsObject.path+fsObject.fileName, fsObject);

        }

        if(hasMore){

            MSRequest* mrq=new MSRequest();

            QString nl=job["@odata.nextLink"].toString();

            mrq->addHeader("Authorization",                     "Bearer "+this->access_token);

            mrq->raw_exec(nl);

            if(!mrq->replyOK()){
                mrq->printReplyError();
                delete(mrq);
                return false;
            }


            content= mrq->replyText;


            json = QJsonDocument::fromJson(content.toUtf8());

            job = json.object();
            items=job["value"].toArray();

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

    if(! this->getFlag("noHidden")){// if show hidden
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

            if(this->getFlag("useInclude") && this->includeList != ""){//  --use-include

                if( this->filterIncludeFileNames(relPath)){

                    continue;
                }
            }
            else{// use exclude by default

                if(this->excludeList != ""){
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

                if(relPath.lastIndexOf("/")==0){
                    fsObject.path="/";
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf("/"))+"/";
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


            if(this->getFlag("useInclude") && this->includeList != ""){//  --use-include

                if( this->filterIncludeFileNames(relPath)){

                    return false;
                }
            }
            else{// use exclude by default

                if(this->excludeList != ""){
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

                if(relPath.lastIndexOf("/")==0){
                    fsObject.path="/";
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf("/"))+"/";
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

    QJsonValue s=remoteObject.toObject()["folder"];

    if(s != QJsonValue()){
        return true;
    }

    return false;
}



bool MSOneDrive::isFile(const QJsonValue &remoteObject){

    QJsonValue s=remoteObject.toObject()["file"];

    if(s != QJsonValue()){
        return true;
    }

    return false;

}

bool MSOneDrive::createSyncFileList(){

    if(this->getFlag("useInclude")){
        QFile key(this->workPath+"/.include");

        if(key.open(QIODevice::ReadOnly)){

            QTextStream instream(&key);
            QString line;
            while(!instream.atEnd()){

                line=instream.readLine();
                if(line.isEmpty()){
                    continue;
                }
                if(instream.pos() == 9 && line == "wildcard"){
                    this->options.insert("filter-type", "wildcard");
                    continue;
                }
                else if(instream.pos() == 7 && line == "regexp"){
                    this->options.insert("filter-type", "regexp");
                    continue;
                }
                this->includeList=this->includeList+line+"|";
            }
            this->includeList=this->includeList.left(this->includeList.size()-1);

            QRegExp regex2(this->excludeList);
            if(this->getOption("filter-type") == "regexp")
                regex2.setPatternSyntax(QRegExp::RegExp);
            else
                regex2.setPatternSyntax(QRegExp::Wildcard);
            if(!regex2.isValid()){
                qStdOut()<<"Include filelist contains errors. Program will be terminated.";
                return false;
            }
        }
    }
    else{
        QFile key(this->workPath+"/.exclude");

        if(key.open(QIODevice::ReadOnly)){

            QTextStream instream(&key);
            QString line;
            while(!instream.atEnd()){

                line=instream.readLine();
                if(instream.pos() == 9 && line == "wildcard"){
                    this->options.insert("filter-type", "wildcard");
                    continue;
                }
                else if(instream.pos() == 7 && line == "regexp"){
                    this->options.insert("filter-type", "regexp");
                    continue;
                }
                if(line.isEmpty()){
                    continue;
                }
                this->excludeList=this->excludeList+line+"|";
            }
            this->excludeList=this->excludeList.left(this->excludeList.size()-1);

            QRegExp regex2(this->excludeList);
            if(this->getOption("filter-type") == "regexp")
                regex2.setPatternSyntax(QRegExp::RegExp);
            else
                regex2.setPatternSyntax(QRegExp::Wildcard);
            if(!regex2.isValid()){
                qStdOut()<<"Exclude filelist contains errors. Program will be terminated.";
                return false;
            }
        }
    }

    qStdOut()<< "Reading remote files"<<endl;


    if(!this->readRemote("")){// top level files and folders
        qStdOut()<<"Error occured on reading remote files" <<endl;
        return false;

    }

    qStdOut()<<"Reading local files and folders" <<endl;

    if(!this->readLocal(this->workPath)){
        qStdOut()<<"Error occured on local files and folders" <<endl;
        return false;

    }

//this->remote_file_get(&(this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::file).begin().value()));//&(this->syncFileList.values()[1])
//this->remote_file_insert(&(this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::file).begin().value()));
//this->remote_file_update(&(this->syncFileList.values()[0]));
// this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
//    this->remote_file_trash(&(this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::file).begin().value()));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));


    this->doSync();

    return true;

}


// TODO: Need way how does it
bool MSOneDrive::directUpload(const QString &url, const QString &remotePath){
Q_UNUSED(remotePath);
Q_UNUSED(url);

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.onedrive.com/v1.0/drive/items/D7C91EBFD21F9BA0!510/children");
    req->setMethod("post");

    req->addHeader("Authorization","Bearer "+this->access_token);
    req->addHeader("Content-Type", QString("application/json"));
    req->addHeader("Prefer", QString("respond-async"));


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

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.onedrive.com/v1.0/drive");
    req->setMethod("get");

    req->addHeader("Authorization","Bearer "+this->access_token);

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return "false";
    }



    QString content= req->replyText;//lastReply->readAll();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    delete(req);

    QJsonObject out;
    out["account"]=job["owner"].toObject()["user"].toObject()["displayName"].toString();
    out["total"]= QString::number( (uint64_t)job["quota"].toObject()["total"].toDouble());
    out["usage"]= QString::number( (uint64_t)job["quota"].toObject()["used"].toDouble());

    return QString( QJsonDocument(out).toJson());

}



bool MSOneDrive::onAuthFinished(const QString &html, MSCloudProvider *provider){

    Q_UNUSED(provider);

        MSRequest* req=new MSRequest(this->proxyServer);

        req->setRequestUrl("https://login.live.com/oauth20_token.srf");
        req->setMethod("post");

        req->addQueryItem("redirect_uri",           "http://localhost:1973");
        req->addQueryItem("grant_type",          "authorization_code");
        req->addQueryItem("client_id",              "07bcebfb-0764-450a-b9ef-e839c592a418");
        req->addQueryItem("client_secret",              "FFWd2Pc5jbqqmTaknxMEYQ5");
        req->addQueryItem("code",                  html);

        req->exec();


        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }



        QString content= req->replyText;//lastReply->readAll();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();
        QString v=job["refresh_token"].toString();

        if(v!=""){

            this->token=v;
            qStdOut() << "Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize."<<endl;
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
