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

#include "msmailru.h"


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


#define MAILRU_MAX_FILESIZE  8000000


MSMailRu::MSMailRu()
{
    this->providerName=     "MailRu";
    this->tokenFileName=    ".mailru";
    this->stateFileName=    ".mailru_state";
    this->trashFileName=    ".trash_mailru";

    this->cookies=0;
}

//=======================================================================================


bool MSMailRu::auth(){


    MSRequest* req=new MSRequest(this->proxyServer);

    this->cookies=new QNetworkCookieJar();

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("http://auth.mail.ru/cgi-bin/auth?lang=ru_RU&from=authpopup");
    req->setMethod("post");

    req->addQueryItem("page",           "https://cloud.mail.ru/?from=promo");
    req->addQueryItem("FailPage",       "");
    req->addQueryItem("Domain",         "mail.ru");
    req->addQueryItem("Login",          this->login);
    req->addQueryItem("Password",       this->password);
    req->addQueryItem("new_auth_form",  "1");

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        this->providerAuthStatus=false;
        return false;
    }

    //req->cookieToJSON();

    QString r=req->replyText;

    int start =  r.indexOf("\"csrf\"");
    QString r_out;

    //delete(req);

    if(start > 0){

        start=start+8;
        r_out= r.mid(start,32);
        this->providerAuthStatus=true;
        this->token=r_out;


        start =r.indexOf("\"x-page-id\":");
        if(start >0){

            start+=13;
            r_out=r.mid(start,10);

            this->x_page_id=r_out;
        }
        else{
            this->providerAuthStatus=false;
            delete(req);
            return false;
        }

        start =r.indexOf("\"BUILD\":");
        if(start >0){

            start+=9;
            QString r_tmp=r.mid(start,100);
            int end=r.indexOf("\"",start);

            r_out=r_tmp.mid(0,end-start);

            this->build=r_out;
        }
        else{
            this->providerAuthStatus=false;
            delete(req);
            return false;
        }


        //this->cookies=new QNetworkCookieJar(req->manager->cookieJar());
        this->cookies=(req->manager->cookieJar());

        //this->readRemote("/");

        //delete(req);
        this->providerAuthStatus=true;
        req->deleteLater();
        return true;
    }
    else{
        this->providerAuthStatus=false;
        delete(req);
        return false;
    }




}

//=======================================================================================


void MSMailRu::saveTokenFile(const QString &path)
{
    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"access_token\" : \""+this->token+"\", \"build\": \""+this->build+"\", \"x_page_id\": \""+this->x_page_id+"\", \"login\": \""+this->login+"\", \"password\": \""+this->password+"\" }";
    key.close();
}

//=======================================================================================


bool MSMailRu::loadTokenFile(const QString &path)
{
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
    QString v=job["access_token"].toString();

    this->token=v;
    this->build=job["build"].toString();
    this->x_page_id=job["x_page_id"].toString();
    this->login=job["login"].toString();
    this->password=job["password"].toString();

    key.close();
    return true;
}

//=======================================================================================


void MSMailRu::loadStateFile()
{
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

//=======================================================================================


void MSMailRu::saveStateFile()
{

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

//=======================================================================================


bool MSMailRu::refreshToken()
{
    this->access_token=this->token;
    return true;
}

//=======================================================================================

QString MSMailRu::remote_dispatcher(const QString &target){

    this->auth();


    MSRequest* req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("https://cloud.mail.ru/api/v2/dispatcher");
    req->setMethod("get");

    req->addQueryItem("token",       this->token);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return "";
    }

    QString content=req->readReplyText();

    this->cookies=req->manager->cookieJar();


    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString serverURL=job["body"].toObject()[target].toArray()[0].toObject()["url"].toString();

    req->deleteLater();

    return serverURL;

}


//=======================================================================================


bool MSMailRu::remote_file_get(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return true;
    }

    if(object->remote.objectType == MSRemoteFSObject::Type::folder){
        return true;
    }

    QString id=object->remote.data["home"].toString();

    this->auth();

    MSRequest* req_prev;
    MSRequest* req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("https://cloud.mail.ru/api/v2/dispatcher");
    req->setMethod("get");

    req->addQueryItem("token",       this->token);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    QString content=req->readReplyText();

    this->cookies=req->manager->cookieJar();
    req_prev=req;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString serverURL=job["body"].toObject()["get"].toArray()[0].toObject()["url"].toString();


        req=new MSRequest(this->proxyServer);

        req->MSsetCookieJar(this->cookies);

        req->setRequestUrl(serverURL+object->remote.data["home"].toString().mid(1));
        req->setMethod("get");

        req->addQueryItem("x-email",      this->login);

        req->exec();

        delete(req_prev);


    QString filePath=this->workPath+object->path+object->fileName;


    if(this->testReplyBodyForError(req->replyErrorText)){

        if(object->remote.objectType==MSRemoteFSObject::Type::file){


            this->local_writeFileContent(filePath,req);
            // set remote "change time" for local file

            utimbuf tb;

            double zz=object->remote.data["mtime"].toDouble();
            QDateTime zzd=QDateTime::fromTime_t(zz);
            //fsObject.remote.modifiedDate=this->toMilliseconds(zzd,true);

            tb.actime=(this->toMilliseconds(zzd,true))/1000;
            tb.modtime=(this->toMilliseconds(zzd,true))/1000;

            utime(filePath.toStdString().c_str(),&tb);
        }
    }
    else{


            qStdOut() << "Service error. "<< req->replyErrorText;
            delete(req);
            return false;

    }


    delete(req);
    return true;


}

//=======================================================================================


bool MSMailRu::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return true;
    }

    if(this->getFlag("dryRun")){
        return true;
    }


    // get shard server address
    QString url=this->remote_dispatcher("upload");

    MSRequest* req_prev;
    MSRequest* req=new MSRequest(this->proxyServer);

    QString bound="ccross-data";

    req->MSsetCookieJar(this->cookies);

    // upload file body to intermediate server and recive confirm hash

    req->setRequestUrl(url); //url "https://cloclo3-upload.cloud.mail.ru/upload/"
    req->setMethod("post");

    req->addHeader("Content-Type", "multipart/form-data; boundary=----"+QString(bound).toLocal8Bit());

    req->addQueryItem("cloud_domain",       "2");
    req->addQueryItem("fileapi14785802593646",       "");

    req->addQueryItem("x-email",      this->login);


    QByteArray mediaData;

    mediaData.append(QString("------"+bound+"\r\n").toLocal8Bit());
    mediaData.append(QString("Content-Disposition: form-data; name=\"file\"; filename=\""+object->fileName+"\"\r\n"));
    mediaData.append(QString("Content-Type: application/octet-stream\r\n\r\n").toLocal8Bit());
    //mediaData.append(QString("Content-Type: multipart/form-data;\r\n\r\n").toLocal8Bit());


    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }
    mediaData.append(file.readAll());

    file.close();

    mediaData.append(QString("\r\n------"+bound+"--\r\n").toLocal8Bit());

    req->addHeader("Content-Length",QString::number(mediaData.length()).toLocal8Bit());

    req->post(mediaData);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }


    QStringList arr=QString(req->readReplyText()).split(';'); // at this point we recieve confirmation hash in arr[0] and size of file in arr[1]

    this->cookies=req->manager->cookieJar();
    req_prev=req;

    req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);

    delete(req_prev);


    req->setRequestUrl("https://cloud.mail.ru/api/v2/file/add");
    req->setMethod("post");

    req->addQueryItem("api",       "2");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("conflict",       "strict"); // api does not contains ignore param CLOUDWEB-5028 (7114 too)
    req->addQueryItem("email",       this->login);
    req->addQueryItem("home",       object->path+object->fileName);
    req->addQueryItem("hash",       arr[0]);
    req->addQueryItem("size",       arr[1].left(arr[1].length() - 2));
    req->addQueryItem("token",       this->token);
    req->addQueryItem("x-email",       this->login);
    req->addQueryItem("x-page-id",       this->x_page_id);

    req->exec();

    if(!req->replyOK()){

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();
        QString error=job["body"].toObject()["home"].toObject()["error"].toString();

        if(error == "exists"){// need remove and re-upload

            bool r=this->remote_file_trash(object);
            if(r == true){

                this->remote_file_insert(object);
                delete(req);
                return true;

            }


        }



        req->printReplyError();
        delete(req);
        return false;
    }

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    int status=job["status"].toInt();

    delete(req);

    if(status == 200){

        // set time for local file to tome of remote file to prevent re-downloading
        utimbuf tb;
        QString filePath=this->workPath+object->path+object->fileName;


        double zz=job["time"].toDouble();
        QDateTime zzd=QDateTime::fromTime_t(zz/1000);
        //fsObject.remote.modifiedDate=this->toMilliseconds(zzd,true);

        tb.actime=(this->toMilliseconds(zzd,true))/1000;
        tb.modtime=(this->toMilliseconds(zzd,true))/1000;

        utime(filePath.toStdString().c_str(),&tb);

        return true;
    }
    else{
        return false;
    }

}

//=======================================================================================


bool MSMailRu::remote_file_update(MSFSObject *object)
{
    return this->remote_file_insert(object);
}

//=======================================================================================


//bool MSMailRu::remote_file_generateIDs(int count)
//{
//Q_UNUSED(count);
//return true;
//}

//=======================================================================================


bool MSMailRu::remote_file_makeFolder(MSFSObject *object){

    this->auth();

    MSRequest* req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);


    req->setRequestUrl("https://cloud.mail.ru/api/v2/folder/add");
    req->setMethod("post");

    req->addQueryItem("api",       "2");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("conflict",       "strict");// api does not contains ignore param CLOUDWEB-5028 (7114 too)
    req->addQueryItem("email",       this->login);
    req->addQueryItem("home",        object->path+object->fileName); //object->path+object->fileName
    req->addQueryItem("token",       this->token);
    req->addQueryItem("x-email",     this->login);
    req->addQueryItem("x-page-id",   this->x_page_id);

    req->exec();

    if(!req->replyOK()){

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();
        QString error=job["body"].toObject()["home"].toObject()["error"].toString();

        if(error == "exists"){// need remove and re-upload

            bool r=this->remote_file_trash(object);
            if(r == true){

                this->remote_file_makeFolder(object);
                delete(req);
                return true;

            }


        }

        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    int status=job["status"].toInt();

    delete(req);

    if(status == 200){
        return true;
    }
    else{
        return false;
    }

}

//=======================================================================================


void MSMailRu::remote_file_makeFolder(MSFSObject *object, const QString &parentID)
{
Q_UNUSED(object);

Q_UNUSED(parentID);
}

//=======================================================================================


bool MSMailRu::remote_file_trash(MSFSObject *object){

    this->auth();

    MSRequest* req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);


    req->setRequestUrl("https://cloud.mail.ru/api/v2/file/remove");
    req->setMethod("post");

    req->addQueryItem("api",       "2");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("email",       this->login);
    req->addQueryItem("home",        object->path+object->fileName);
    req->addQueryItem("token",       this->token);
    req->addQueryItem("x-email",     this->login);
    req->addQueryItem("x-page-id",   this->x_page_id);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    int status=job["status"].toInt();

    delete(req);

    if(status == 200){
        return true;
    }
    else{
        return false;
    }


}


//=======================================================================================

bool MSMailRu::remote_createDirectory(const QString &path){

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

//=======================================================================================


void MSMailRu::local_createDirectory(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================


void MSMailRu::local_removeFile(const QString &path){


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

//=======================================================================================


void MSMailRu::local_removeFolder(const QString &path){


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

//=======================================================================================


MSFSObject::ObjectState MSMailRu::filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote){


    if((local.exist)&&(remote.exist)){ //exists both files

//        if(local.md5Hash==remote.md5Hash){


//                return MSFSObject::ObjectState::Sync;

//        }
//        else{

            // compare last modified date for local and remote
            if(local.modifiedDate==remote.modifiedDate){

                return MSFSObject::ObjectState::Sync;

//                if(this->strategy==MSCloudProvider::SyncStrategy::PreferLocal){
//                    return MSFSObject::ObjectState::ChangedLocal;
//                }
//                else{
//                    return MSFSObject::ObjectState::ChangedRemote;
//                }

            }
            else{

                if(local.objectType == MSLocalFSObject::Type::folder){
                    return MSFSObject::ObjectState::Sync;
                }

                if(local.modifiedDate > remote.modifiedDate){
                    return MSFSObject::ObjectState::ChangedLocal;
                }
                else{
                    return MSFSObject::ObjectState::ChangedRemote;
                }

            }
        }


    //}


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

//=======================================================================================


void MSMailRu::doSync(){

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

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByState(MSFSObject::ObjectState state){

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

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByState( QHash<QString, MSFSObject> fsObjectList, MSFSObject::ObjectState state){

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

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type){

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

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type){

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

//=======================================================================================


bool MSMailRu::filelist_FSObjectHasParent(const MSFSObject &fsObject){

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

//=======================================================================================


MSFSObject MSMailRu::filelist_getParentFSObject(const MSFSObject &fsObject){

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

//=======================================================================================


void MSMailRu::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}

//=======================================================================================


bool MSMailRu::testReplyBodyForError(const QString &body){

    if(body.contains("Error transferring ")){

        return false;

    }
    else{
        return true;
    }
}

//=======================================================================================


QString MSMailRu::getReplyErrorString(const QString &body){

    return body;

}

//=======================================================================================


bool MSMailRu::createHashFromRemote(){
    return true;
}


//=======================================================================================

bool MSMailRu::readRemote(const QString &path, QNetworkCookieJar* cookie)
{

    if(cookie == NULL){

        this->auth();

        if(this->providerAuthStatus == false){

            qStdOut()<<"Authentication failed. Possibly you need re-auth or login and passwrd is incorrect" <<endl;

            return providerAuthStatus;
        }
    }

    //MSRequest* req_prev;

    MSRequest* req=new MSRequest(this->proxyServer);

    if(cookie == NULL){
        req->MSsetCookieJar(this->cookies);
    }
    else{
        req->MSsetCookieJar(cookie);
    }

    req->setRequestUrl("https://cloud.mail.ru/api/v2/folder");
    req->setMethod("get");

    req->addQueryItem("token",       this->token);
    req->addQueryItem("home",       path);
    req->addQueryItem("build",       this->build);
    req->addQueryItem("x-page-id",      this->x_page_id);
    req->addQueryItem("api",      "2");
    req->addQueryItem("offset",      "0");
    req->addQueryItem("limit",      "2000000");
    req->addQueryItem("email",      this->login);
    req->addQueryItem("x-email",      this->login);
    req->addQueryItem("_",      "1433249148810");


    req->exec();

    if(!req->replyOK()){
        delete(req);
        req->printReplyError();
        return false;
    }

    QString list=req->readReplyText();


    QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = json.object();
    QJsonArray entries=job["body"].toObject()["list"].toArray();


        for(int i=0; i < entries.size(); i++){

            QJsonObject o=entries[i].toObject();


            MSFSObject fsObject;

            fsObject.path = QFileInfo(o["home"].toString()).path()+"/";

            if(fsObject.path == "//"){
               fsObject.path ="/";
            }

            if(o["type"] == QString("folder")){

                fsObject.remote.data=o;
                fsObject.remote.exist=true;
                fsObject.isDocFormat=false;
                //fsObject.remote.md5Hash=o["hash"].toString();

                fsObject.state=MSFSObject::ObjectState::NewRemote;

                fsObject.fileName=o["name"].toString();
                fsObject.remote.objectType=MSRemoteFSObject::Type::folder;

                this->readRemote(fsObject.path+fsObject.fileName,0);//req->manager->cookieJar()

            }
            else{

                fsObject.remote.fileSize=  o["size"].toInt();
                fsObject.remote.data=o;
                fsObject.remote.exist=true;
                fsObject.isDocFormat=false;
                //fsObject.remote.md5Hash=o["hash"].toString();

                fsObject.state=MSFSObject::ObjectState::NewRemote;

                fsObject.fileName=o["name"].toString();
                fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                double zz=o["mtime"].toDouble();
                QDateTime zzd=QDateTime::fromTime_t(zz);
                fsObject.remote.modifiedDate=this->toMilliseconds(zzd,true);
            }



            if(! this->filterServiceFileNames(o["home"].toString())){// skip service files and dirs

                continue;
            }


            if(this->getFlag("useInclude")){//  --use-include

                if( this->filterIncludeFileNames(o["home"].toString())){

                    continue;
                }
            }
            else{// use exclude by default

                if(! this->filterExcludeFileNames(o["home"].toString())){

                    continue;
                }
            }

            this->syncFileList.insert(o["home"].toString(), fsObject);


        }

    delete(req);

        return true;
}

bool MSMailRu::_readRemote(const QString &rootPath){

    Q_UNUSED(rootPath);
    return this->readRemote("/",NULL);
}


//=======================================================================================


bool MSMailRu::readLocal(const QString &path){


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

            if(this->getFlag("useInclude")){//  --use-include

                if( this->filterIncludeFileNames(relPath)){
                    continue;
                }
            }
            else{// use exclude by default

                if(! this->filterExcludeFileNames(relPath)){
                    continue;
                }
            }



            QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



            if(i!=this->syncFileList.end()){// if object exists in Google Drive

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                //fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
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
                //fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
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

//=======================================================================================


bool MSMailRu::readLocalSingle(const QString &path){

        QFileInfo fi(path);



            QString relPath=fi.absoluteFilePath().replace(this->workPath,"");

            if(! this->filterServiceFileNames(relPath)){// skip service files and dirs
                return false;
            }


            if(this->getFlag("useInclude")){//  --use-include

                if( this->filterIncludeFileNames(relPath)){
                    return false;
                }
            }
            else{// use exclude by default

                if(! this->filterExcludeFileNames(relPath)){
                    return false;
                }
            }



            QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



            if(i!=this->syncFileList.end()){// if object exists in Google Drive

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                //fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
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
                //fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Sha1);
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

//=======================================================================================


bool MSMailRu::isFolder(const QJsonValue &remoteObject){

    if(remoteObject.toObject()[".tag"].toString()=="folder"){
        return true;
    }
    return false;
}

//=======================================================================================


bool MSMailRu::isFile(const QJsonValue &remoteObject){

    if(remoteObject.toObject()[".tag"].toString()=="file"){
        return true;
    }
    return false;
}

//=======================================================================================

bool MSMailRu::createSyncFileList(){

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


    //this->createHashFromRemote();

    // begin create



    if(!this->readRemote("/",NULL)){// top level files and folders

        qStdOut()<<"Error occured on reading remote files" <<endl;
        return false;
    }

    qStdOut()<<"Reading local files and folders" <<endl;

    if(!this->readLocal(this->workPath)){

        qStdOut()<<"Error occured on local files and folders" <<endl;
        return false;
    }

//this->remote_file_get(&(this->syncFileList.values()[0]));
//this->remote_file_insert(&(this->syncFileList.values()[0]));
//this->remote_file_update(&(this->syncFileList.values()[0]));
// this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));


    this->doSync();

    return true;
}

//=======================================================================================


bool MSMailRu::directUpload(const QString &url, const QString &remotePath){

    // download file into temp file ---------------------------------------------------------------

    MSRequest *reqd = new MSRequest(this->proxyServer);

    QString filePath=this->workPath+"/"+this->generateRandom(10);

    reqd->setMethod("get");
    reqd->download(url,filePath);


    if(!reqd->replyOK()){
        reqd->printReplyError();
        delete(reqd);
        exit(1);
    }

    QFileInfo fileRemote(remotePath);
    QString path=fileRemote.absoluteFilePath();

    path=QString(path).left(path.lastIndexOf("/"));

    MSFSObject object;
    object.path=path+"/";
    object.fileName=fileRemote.fileName();

    this->syncFileList.insert(object.path+object.fileName,object);

    if(path!="/"){

        QStringList dirs=path.split("/");

        MSFSObject* obj=0;
        QString cPath="/";

        for(int i=1;i<dirs.size();i++){

            obj=new MSFSObject();
            obj->path=cPath+dirs[i];
            obj->fileName="";
            obj->remote.exist=false;
            this->remote_file_makeFolder(obj);
            cPath+=dirs[i]+"/";
            delete(obj);

        }


    }



    delete(reqd);

    // upload file to remote ------------------------------------------------------------------------

    // get shard server address
    this->auth();
    QString urld=this->remote_dispatcher("upload");

    MSRequest* req_prev;
    MSRequest* req=new MSRequest(this->proxyServer);

    QString bound="ccross-data";

    req->MSsetCookieJar(this->cookies);

    // upload file body to intermediate server and recive confirm hash

    req->setRequestUrl(urld); //url "https://cloclo3-upload.cloud.mail.ru/upload/"
    req->setMethod("post");

    req->addHeader("Content-Type", "multipart/form-data; boundary=----"+QString(bound).toLocal8Bit());

    req->addQueryItem("cloud_domain",       "2");
    req->addQueryItem("fileapi14785802593646",       "");

    req->addQueryItem("x-email",      this->login);


    QByteArray mediaData;

    mediaData.append(QString("------"+bound+"\r\n").toLocal8Bit());
    mediaData.append(QString("Content-Disposition: form-data; name=\"file\"; filename=\""+object.fileName+"\"\r\n"));
    mediaData.append(QString("Content-Type: application/octet-stream\r\n\r\n").toLocal8Bit());


    // read file content and put him into request body


    QFile file(filePath);


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }
    mediaData.append(file.readAll());

    file.close();

    mediaData.append(QString("\r\n------"+bound+"--\r\n").toLocal8Bit());

    req->addHeader("Content-Length",QString::number(mediaData.length()).toLocal8Bit());

    req->post(mediaData);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }


    QStringList arr=QString(req->readReplyText()).split(';'); // at this point we recieve confirmation hash in arr[0] and size of file in arr[1]

    this->cookies=req->manager->cookieJar();
    req_prev=req;

    req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);

    delete(req_prev);


    req->setRequestUrl("https://cloud.mail.ru/api/v2/file/add");
    req->setMethod("post");

    req->addQueryItem("api",       "2");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("conflict",       "rename"); // api does not contains ignore param CLOUDWEB-5028 (7114 too)
    req->addQueryItem("email",       this->login);
    req->addQueryItem("home",       object.path+object.fileName);
    req->addQueryItem("hash",       arr[0]);
    req->addQueryItem("size",       arr[1].left(arr[1].length() - 2));
    req->addQueryItem("token",       this->token);
    req->addQueryItem("x-email",       this->login);
    req->addQueryItem("x-page-id",       this->x_page_id);

    req->exec();

    if(!req->replyOK()){

        QString content=req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();
        QString error=job["body"].toObject()["home"].toObject()["error"].toString();



        req->printReplyError();
        delete(req);
        return false;
    }

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    int status=job["status"].toInt();

    file.remove();

    delete(req);

    if(status == 200){



        return true;
    }
    else{
        return false;
    }


}

QString MSMailRu::getInfo(){

    this->auth();


    MSRequest* req=new MSRequest(this->proxyServer);

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("https://cloud.mail.ru/api/v2/user/space");
    req->setMethod("get");

    req->addQueryItem("token",       this->token);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return "false";
    }

    QString content=req->readReplyText();


    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    //QString serverURL=job["body"].toObject()["get"].toArray()[0].toObject()["url"].toString();

    delete(req);

    QJsonObject out; //1048576
    out["account"]=job["email"].toString();
    out["total"]= QString::number(  (uint64_t)job["body"].toObject()["total"].toDouble());
    out["usage"]= QString::number( (uint64_t)job["body"].toObject()["used"].toDouble());

    return QString( QJsonDocument(out).toJson());


}

//=======================================================================================


// not used
bool MSMailRu::onAuthFinished(const QString &html, MSCloudProvider *provider)
{
Q_UNUSED(html);
Q_UNUSED(provider);

    return true;

}
