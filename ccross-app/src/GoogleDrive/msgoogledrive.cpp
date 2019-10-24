/*
    CloudCross: Opensource program for syncronization of local files and folders with clouds

    Copyright (C) 2016-2017  Vladimir Kamensky
    Copyright (C) 2016-2017  Master Soft LLC.
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



#include "msgoogledrive.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "iostream"


MSGoogleDrive::MSGoogleDrive() :
    MSCloudProvider()
{
    this->providerName="GoogleDrive";
    this->tokenFileName=".grive";
    this->stateFileName=".grive_state";
    this->trashFileName=".trash";


}

//=======================================================================================

bool MSGoogleDrive::auth(){


    connect(this,SIGNAL(oAuthCodeRecived(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));
    connect(this,SIGNAL(oAuthError(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));


    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"));
    req->setMethod(QStringLiteral("get"));

    req->addQueryItem(QStringLiteral("scope"),                  QStringLiteral("https://www.googleapis.com/auth/drive+https://www.googleapis.com/auth/userinfo.email+https://www.googleapis.com/auth/userinfo.profile+https://docs.google.com/feeds/+https://docs.googleusercontent.com/+https://spreadsheets.google.com/feeds/"));
    req->addQueryItem(QStringLiteral("redirect_uri"),           QStringLiteral("http://127.0.0.1:1973"));
    req->addQueryItem(QStringLiteral("response_type"),          QStringLiteral("code"));
    req->addQueryItem(QStringLiteral("client_id"),              QStringLiteral("834415955748-oq0p2m5dro2bvh3bu0o5bp19ok3qrs3f.apps.googleusercontent.com"));
    req->addQueryItem(QStringLiteral("access_type"),            QStringLiteral("offline"));
    req->addQueryItem(QStringLiteral("approval_prompt"),        QStringLiteral("force"));
    req->addQueryItem(QStringLiteral("state"),                  QStringLiteral("1"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        this->providerAuthStatus=false;
        return false;
    }

    this->startListener(1973);

    //    if(!this->testReplyBodyForError(req->readReplyText())){
    //        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
    //        exit(0);
    //    }


    qStdOut()<< QStringLiteral("-------------------------------------")<<endl ;
    qStdOut()<< tr("Please go to this URL and confirm application credentials") <<endl ;

    qStdOut() << req->replyURL()<<endl;
    qStdOut()<<"" <<endl;
    ;

    delete(req);

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

//=======================================================================================

bool MSGoogleDrive::onAuthFinished(const QString &html, MSCloudProvider *provider){

    Q_UNUSED(provider)

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);


    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/oauth2/v4/token"));
    req->setMethod(QStringLiteral("post"));

    req->addQueryItem(QStringLiteral("client_id"),          QStringLiteral("834415955748-oq0p2m5dro2bvh3bu0o5bp19ok3qrs3f.apps.googleusercontent.com"));
    req->addQueryItem(QStringLiteral("client_secret"),      QStringLiteral("YMBWydU58CvF3UP9CSna-BwS"));
    req->addQueryItem(QStringLiteral("code"),               html.trimmed());
    req->addQueryItem(QStringLiteral("grant_type"),         QStringLiteral("authorization_code"));
    req->addQueryItem(QStringLiteral("redirect_uri"),           QStringLiteral("http://127.0.0.1:1973"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        this->providerAuthStatus=false;
        emit providerAuthComplete();
        return false;
    }


    QString content= req->replyText;//lastReply->readAll();

    //qStdOut() << content;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job[QStringLiteral("refresh_token")].toString();

    delete(req);

    if(v!=""){

        this->token=v;
        qStdOut() << QStringLiteral("Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize.") <<endl;
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


//=======================================================================================


void MSGoogleDrive::saveTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << QStringLiteral("{\"refresh_token\" : \"") + this->token + QStringLiteral("\"}");
    key.close();
}

//=======================================================================================

bool MSGoogleDrive::loadTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);

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

//=======================================================================================

void MSGoogleDrive::loadStateFile(){

    QFile key(this->credentialsPath+"/"+this->stateFileName);

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

}

//=======================================================================================

void MSGoogleDrive::saveStateFile(){


    QJsonDocument state;
    QJsonObject jso;
    jso.insert(QStringLiteral("change_stamp"),QStringLiteral("0"));

    QJsonObject jts;
    jts.insert(QStringLiteral("nsec"),QStringLiteral("0"));
    jts.insert(QStringLiteral("sec"),QString::number(QDateTime( QDateTime::currentDateTime()).toMSecsSinceEpoch()));

    jso.insert(QStringLiteral("last_sync"),jts);
    state.setObject(jso);

    QFile key(this->credentialsPath + QStringLiteral("/") + this->stateFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << state.toJson();
    key.close();

}


//=======================================================================================

bool MSGoogleDrive::refreshToken(){

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/oauth2/v4/token"));
    req->setMethod(QStringLiteral("post"));

    req->addQueryItem(QStringLiteral("refresh_token"),          this->token);
    req->addQueryItem(QStringLiteral("client_id"),              QStringLiteral("834415955748-oq0p2m5dro2bvh3bu0o5bp19ok3qrs3f.apps.googleusercontent.com"));
    req->addQueryItem(QStringLiteral("client_secret"),          QStringLiteral("YMBWydU58CvF3UP9CSna-BwS"));
    req->addQueryItem(QStringLiteral("grant_type"),             QStringLiteral("refresh_token"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        return false;
    }


    QString content= req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job[QStringLiteral("access_token")].toString();

    if(v!=""){
        this->access_token=v;

        //        this->token=v;

        //        this->saveTokenFile(this->workPath);


        delete(req);
        return true;
    }
    else{
        delete(req);
        return false;
    }

}

//=======================================================================================

bool MSGoogleDrive::createHashFromRemote(){

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files"));
    req->setMethod(QStringLiteral("get"));

    req->addQueryItem(QStringLiteral("access_token"),           this->access_token);

    req->addQueryItem(QStringLiteral("q"), QStringLiteral("trashed=false"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }


    QString list=req->readReplyText();


    // collect all files in GoogleDrive to array driveJSONFileList
    QJsonDocument jsonList = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = jsonList.object();

    do{

        delete(req);

        req=new MSHttpRequest(this->proxyServer);


        req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files"));
        req->setMethod(QStringLiteral("get"));

        req->addQueryItem(QStringLiteral("access_token"),           this->access_token);

        if(this->getFlag("lowMemory")){
            req->addQueryItem(QStringLiteral("maxResults"),           QStringLiteral("100"));
        }
        else{
            req->addQueryItem(QStringLiteral("maxResults"),           QStringLiteral("1000"));
        }

        req->addQueryItem(QStringLiteral("q"), QStringLiteral("trashed=false"));

        QString nextPageToken=job[QStringLiteral("nextPageToken")].toString();

        QJsonArray items=job[QStringLiteral("items")].toArray();

        for(int i=0;i<items.size();i++){

            driveJSONFileList.insert(items[i].toObject()["id"].toString(),items[i]);

        }

        req->addQueryItem(QStringLiteral("pageToken"),           nextPageToken);

        req->exec();


        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
            delete(req);
            return false;
        }


        list=req->readReplyText();

        jsonList = QJsonDocument::fromJson(list.toUtf8());



        job = jsonList.object();

        if(job[QStringLiteral("nextPageToken")].toString() == ""){ // last part of data


            QJsonArray items=job[QStringLiteral("items")].toArray();

            for(int i=0;i<items.size();i++){

                driveJSONFileList.insert(items[i].toObject()[QStringLiteral("id")].toString(),items[i]);

            }
        }


    }while(job[QStringLiteral("nextPageToken")].toString()!=""); //while(false);



    delete(req);
    return true;
}


//=======================================================================================

QString MSGoogleDrive::getRoot(){


    MSHttpRequest* req = new MSHttpRequest(this->proxyServer);
    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files/root/children"));
    req->setMethod(QStringLiteral("get"));

    req->addQueryItem(QStringLiteral("maxResults"), QStringLiteral("2"));
    req->addHeader(QStringLiteral("Authorization"), QStringLiteral("Bearer ") + this->access_token);

    req->exec();

    QString list=req->readReplyText();

    QJsonDocument jsonList = QJsonDocument::fromJson(list.toUtf8());

    QJsonObject job = jsonList.object();

    QString sl = job[QStringLiteral("items")].toArray()[0].toObject()[QStringLiteral("selfLink")].toString();

    return sl.mid(sl.indexOf(QStringLiteral("/files/"))+7,
                  sl.indexOf(QStringLiteral("/children")) - sl.indexOf(QStringLiteral("/files/")) -7);


    //    QHash<QString,QJsonValue>::iterator i=driveJSONFileList.begin();
    //    QJsonValue v;

    //    QHash<QString,QJsonValue> out;

    //    while(i != driveJSONFileList.end()){

    //        v= i.value();

    //        bool isRoot=v.toObject()["parents"].toArray()[0].toObject()["isRoot"].toBool();

    //        if(isRoot){

    //            QString yy = v.toObject()["parents"].toArray()[0].toObject()["id"].toString();

    //        }

    //       i++;

    //    }

    //    return "";

}


//=======================================================================================

QHash<QString,QJsonValue> MSGoogleDrive::get(const QString &parentId, int target){

    bool files=target & 1;
    bool folders=target & 2;
    bool foldersAndFiles =false;
    bool noTrash = target & 4;

    if(folders && files){
        foldersAndFiles=true;
    }


    QHash<QString,QJsonValue>::iterator i=driveJSONFileList.begin();
    QJsonValue v;

    QHash<QString,QJsonValue> out;

    while(i != driveJSONFileList.end()){

        v= i.value();

        if(v.toObject()[QStringLiteral("parents")].toArray().size()==0){
            if(i != driveJSONFileList.end()){
                i++;
                continue;
            }
        }
        QString oParentId=  v.toObject()[QStringLiteral("parents")].toArray()[0].toObject()[QStringLiteral("id")].toString();
        QString oMimeType=  v.toObject()[QStringLiteral("mimeType")].toString();
        //bool    oTrashed=   v.toObject()["labels"].toArray()[0].toObject()["trashed"].toBool();
        bool    oTrashed=   v.toObject()[QStringLiteral("labels")].toObject()[QStringLiteral("trashed")].toBool();

        if(oParentId == parentId){

            if(files && !foldersAndFiles){

                if(oMimeType != QStringLiteral("application/vnd.google-apps.folder")){

                    if(!noTrash){
                        out.insert(v.toObject()["id"].toString(),v);
                    }
                    else{
                        if(!oTrashed){
                            out.insert(v.toObject()["id"].toString(),v);
                        }
                    }
                }

            }

            if(folders && !foldersAndFiles){

                if(oMimeType == QStringLiteral("application/vnd.google-apps.folder")){

                    if(!noTrash){
                        out.insert(v.toObject()["id"].toString(),v);
                    }
                    else{
                        if(!oTrashed){
                            out.insert(v.toObject()["id"].toString(),v);
                        }
                    }
                }
            }

            if(foldersAndFiles){

                if(!noTrash){
                    out.insert(v.toObject()[QStringLiteral("id")].toString(),v);
                }
                else{
                    if(!oTrashed){
                        out.insert(v.toObject()[QStringLiteral("id")].toString(),v);
                    }
                }

            }

        }

        i++;

    }


    return out;

}

//=======================================================================================

bool MSGoogleDrive::isFile(const QJsonValue &remoteObject){
    return remoteObject.toObject()[QStringLiteral("mimeType")].toString() != QStringLiteral("application/vnd.google-apps.folder");
}

//=======================================================================================

bool MSGoogleDrive::isFolder(const QJsonValue &remoteObject){
    return remoteObject.toObject()[QStringLiteral("mimeType")].toString() == QStringLiteral("application/vnd.google-apps.folder");
}

//=======================================================================================

bool MSGoogleDrive::readRemote(const QString &parentId, const QString &currentPath){



    QHash<QString,QJsonValue> list=this->get(parentId,MSCloudProvider::CloudObjects::FilesAndFolders | MSCloudProvider::CloudObjects::NoTrash);

    QHash<QString,QJsonValue>::iterator i=list.begin();

    while(i!=list.end()){

        QJsonObject o=i.value().toObject();

        if(o[QStringLiteral("labels")].toObject()[QStringLiteral("trashed")].toBool()){
            i++;
            continue; // skip trashed objects
        }

        if( this->filterGoogleDocsMimeTypes(o["mimeType"].toString()) ){
            if(!this->getFlag("convertDoc")){
                i++;
                continue; // skip google docs objects if flag convertDoc not set
            }
        }

        MSFSObject fsObject;
        fsObject.path=currentPath;

        fsObject.remote.md5Hash = o[QStringLiteral("md5Checksum")].toString();

        fsObject.remote.fileSize = o[QStringLiteral("fileSize")].toString().toInt();

        fsObject.remote.extraData.insert(QStringLiteral("id"),o[QStringLiteral("id")].toString());
        fsObject.remote.extraData.insert(QStringLiteral("mimeType"),o[QStringLiteral("mimeType")].toString());
        QJsonObject expLink=o[QStringLiteral("exportLinks")].toObject();
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        QVariantHash qh= expLink.toVariantHash();
        fsObject.remote.extraData.insert("exportLinks",QVariant(qh));
#else
        QHash<QString,QVariant> qh;

        QJsonObject::iterator eli=expLink.begin();
        for(;eli != expLink.end();eli++){
            qh.insert(eli.key(),eli.value());
            fsObject.remote.extraData.insert("exportLinks",QVariant(qh));
        }

#endif
        fsObject.remote.extraData.insert("modifiedDate",o[QStringLiteral("modifiedDate")].toString());



        //fsObject.remote.data = o;
        fsObject.remote.exist = true;

        fsObject.state = MSFSObject::ObjectState::NewRemote;

        if(this->getFlag(QStringLiteral("convertDoc")) && this->filterGoogleDocsMimeTypes(o[QStringLiteral("mimeType")].toString())){

            fsObject.isDocFormat = true;

        }


        if(this->isFile(o)){

            fsObject.fileName=o[QStringLiteral("originalFilename")].toString();
            if(fsObject.fileName==QStringLiteral("")){
                fsObject.fileName=o[QStringLiteral("title")].toString();
            }
            fsObject.remote.objectType=MSRemoteFSObject::Type::file;
            fsObject.remote.modifiedDate=(this->toMilliseconds(o[QStringLiteral("modifiedDate")].toString(),true))/1000;
        }

        if(this->isFolder(o)){

            fsObject.fileName=o[QStringLiteral("title")].toString();
            if(fsObject.fileName==""){
                fsObject.fileName=o[QStringLiteral("originalFilename")].toString();
            }
            fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
            fsObject.remote.modifiedDate=(this->toMilliseconds(o[QStringLiteral("modifiedDate")].toString(),true))/1000;

            this->readRemote(o[QStringLiteral("id")].toString(),currentPath+fsObject.fileName+"/");
        }


        if(! this->filterServiceFileNames(currentPath+fsObject.fileName)){// skip service files and dirs
            i++;
            continue;
        }


        //        if(this->isFolder(o)){// recursive self calling
        //            this->readRemote(o[QStringLiteral("id")].toString(),currentPath+fsObject.fileName+"/");
        //        }

        if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

            if( this->filterIncludeFileNames(currentPath+fsObject.fileName)){
                i++;
                continue;
            }
        }
        else{// use exclude by default

            if(this->excludeList != QStringLiteral("")){
                if(! this->filterExcludeFileNames(currentPath+fsObject.fileName)){
                    i++;
                    continue;
                }
            }
        }

        this->syncFileList.insert(currentPath+fsObject.fileName, fsObject);

        //        if(this->isFolder(o)){// recursive self calling
        //            this->readRemote(o["id"].toString(),currentPath+fsObject.fileName+"/");
        //        }


        i++;
    }
    return true;
}

bool MSGoogleDrive::_readRemote(const QString &parentId){
    Q_UNUSED(parentId);

    if(!this->createHashFromRemote()){

        qStdOut() << QStringLiteral("Error occured on reading remote files") <<endl ;
        return false;
    }

    // begin create



    if(!this->readRemote(this->getRoot(),"/")){ // top level files and folders
        qStdOut() << QStringLiteral("Error occured on reading remote files") <<endl ;
        return false;
    }

    return true;

}



//=======================================================================================

bool MSGoogleDrive::readLocal(const QString &path){



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

        MSFSObject tmpObj;
        tmpObj.fileName = relPath;
        QString ext = tmpObj.getObjectExtension();

        if(ext == "xlsx" || ext == "docx" || ext =="pptx" || ext=="odt" || ext=="ods" || ext == "odp"){

            if(this->getFlag(QStringLiteral("convertDoc"))){
                relPath = relPath.left(relPath.lastIndexOf("."));
            }

        }



        QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



        if(i!=this->syncFileList.end()){// if object exists in Google Drive

            MSFSObject* fsObject = &(i.value());


            fsObject->local.fileSize=  fi.size();
            fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
            fsObject->local.exist=true;
            fsObject->getLocalMimeType(this->workPath);

            if(fi.isDir()){
                fsObject->local.objectType=MSLocalFSObject::Type::folder;
                fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;
            }
            else{

                fsObject->local.objectType=MSLocalFSObject::Type::file;
                fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;

            }

            if(this->getFlag(QStringLiteral("convertDoc")) && (this->filterOfficeMimeTypes(fsObject->local.mimeType )||this->filterOfficeFileExtensions(ext/*fsObject->getObjectExtension()*/))){

                fsObject->isDocFormat=true;
            }

            fsObject->state=this->filelist_defineObjectState(fsObject->local,fsObject->remote);

        }
        else{

            MSFSObject fsObject;

            fsObject.state=MSFSObject::ObjectState::NewLocal;

            if(relPath.lastIndexOf(QStringLiteral("/"))==0){
                fsObject.path="/";
            }
            else{
                fsObject.path=QString(relPath).left(relPath.lastIndexOf(QStringLiteral("/")))+QStringLiteral("/");
            }

            fsObject.fileName=fi.fileName();
            fsObject.getLocalMimeType(this->workPath);

            fsObject.local.fileSize=  fi.size();
            fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
            fsObject.local.exist=true;

            if(fi.isDir()){
                fsObject.local.objectType=MSLocalFSObject::Type::folder;
                fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;
            }
            else{

                fsObject.local.objectType=MSLocalFSObject::Type::file;
                fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;

            }

            fsObject.state=this->filelist_defineObjectState(fsObject.local,fsObject.remote);

            if(this->getFlag(QStringLiteral("convertDoc"))  && (this->filterOfficeMimeTypes(fsObject.local.mimeType )||this->filterOfficeFileExtensions(fsObject.getObjectExtension()))){

                fsObject.isDocFormat=true;
            }

            this->syncFileList.insert(relPath,fsObject);

        }


        //            if(fi.isDir()){

        //                readLocal(Path);
        //            }


    }

    return true;

}


//=======================================================================================

bool MSGoogleDrive::readLocalSingle(const QString &path){

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

    MSFSObject tmpObj;
    tmpObj.fileName = relPath;
    QString ext = tmpObj.getObjectExtension();

    if(ext == "xlsx" || ext == "docx" || ext =="pptx" || ext=="odt" || ext=="ods" || ext == "odp"){

        if(this->getFlag(QStringLiteral("convertDoc"))){
            relPath = relPath.left(relPath.lastIndexOf("."));
        }

    }




    QHash<QString,MSFSObject>::iterator i=this->syncFileList.find(relPath);



    if(i!=this->syncFileList.end()){// if object exists in Google Drive

        MSFSObject* fsObject = &(i.value());


        fsObject->local.fileSize=  fi.size();
        fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
        fsObject->local.exist=true;
        fsObject->getLocalMimeType(this->workPath);

        if(fi.isDir()){
            fsObject->local.objectType=MSLocalFSObject::Type::folder;
            fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;
        }
        else{

            fsObject->local.objectType=MSLocalFSObject::Type::file;
            fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;

        }

        if(this->getFlag(QStringLiteral("convertDoc"))  && (this->filterOfficeMimeTypes(fsObject->local.mimeType )||this->filterOfficeFileExtensions(fsObject->getObjectExtension()))){

            fsObject->isDocFormat=true;
        }

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
        fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
        fsObject.local.exist=true;

        if(fi.isDir()){
            fsObject.local.objectType=MSLocalFSObject::Type::folder;
            fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;
        }
        else{

            fsObject.local.objectType=MSLocalFSObject::Type::file;
            fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000;

        }

        fsObject.state=this->filelist_defineObjectState(fsObject.local,fsObject.remote);

        if(this->getFlag(QStringLiteral("convertDoc"))  && (this->filterOfficeMimeTypes(fsObject.local.mimeType )||this->filterOfficeFileExtensions(fsObject.getObjectExtension()))){

            fsObject.isDocFormat=true;
        }

        this->syncFileList.insert(relPath,fsObject);

    }




    return true;

}




//=======================================================================================

bool MSGoogleDrive::filterGoogleDocsMimeTypes(const QString &mime){// return true if this mime is Google document

    return (mime == QStringLiteral("application/vnd.google-apps.document"))
        || (mime == QStringLiteral("application/vnd.google-apps.presentation"))
        || (mime == QStringLiteral("application/vnd.google-apps.spreadsheet"));
}

//=======================================================================================

bool MSGoogleDrive::filterOfficeMimeTypes(const QString &mime){// return true if this mime is Office document

    QRegularExpression regex2(QStringLiteral("word|excel|powerpoint|ppt|xls|doc|opendocument"));

    regex2.patternErrorOffset();

    QRegularExpressionMatch m = regex2.match(mime);

    return m.hasMatch();

}

bool MSGoogleDrive::filterOfficeFileExtensions(QString ext){

    return ext == "xlsx" || ext == "docx" || ext =="pptx" || ext=="odt" || ext=="ods" || ext == "odp";

}

QString MSGoogleDrive::getOfficeFileExtensionByMimeType(QString mimeType){
//if((mime == QStringLiteral("application/vnd.google-apps.document"))||
//        (mime == QStringLiteral("application/vnd.google-apps.presentation"))||
//        (mime == QStringLiteral("application/vnd.google-apps.spreadsheet"))){

    if(mimeType == QStringLiteral("application/vnd.google-apps.document")){
        return "docx";
    }
    if(mimeType == QStringLiteral("application/vnd.google-apps.presentation")){
        return "pptx";
    }
    if(mimeType == QStringLiteral("application/vnd.google-apps.spreadsheet")){
        return "xlsx";
    }
    return "";
}

void MSGoogleDrive::checkFolderStructures(){

    QHash<QString,MSFSObject>::iterator lf;

    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

        // create new folder structure on remote

        qStdOut() << QStringLiteral("Checking folder structure on remote")<<endl  ;

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

        qStdOut() << QStringLiteral("Checking folder structure on local")<<endl  ;

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

//=======================================================================================

//QHash<QString,MSFSObject> MSGoogleDrive::getRemoteFileList(){
bool MSGoogleDrive::getRemoteFileList(){


//    this->createHashFromRemote();
//    this->readRemote(this->getRoot(),"/");// top level files and folders

//    return this->syncFileList;

    if(!this->createHashFromRemote()){
        return false;
    }

    return this->readRemote(this->getRoot(),"/");// top level files and folders
}

//=======================================================================================

bool MSGoogleDrive::createSyncFileList(){

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
                this->excludeList=this->excludeList+line+QStringLiteral("|");
            }
            this->excludeList=this->excludeList.left(this->excludeList.size()-1);

            QRegExp regex2(this->excludeList);
            if(this->getOption(QStringLiteral("filter-type")) == QStringLiteral("regexp"))
                regex2.setPatternSyntax(QRegExp::RegExp);
            else
                regex2.setPatternSyntax(QRegExp::Wildcard);
            if(!regex2.isValid()){
                qStdOut() << QStringLiteral("Exclude filelist contains errors. Program will be terminated.")<<endl;
                return false;
            }
        }
    }

    if(this->getFlag("noSync")){
        qStdOut() << "Synchronization capability was disabled."<<endl;
    }
    else{
        qStdOut()<< QStringLiteral("Reading remote files")<<endl ;

        if(!this->createHashFromRemote()){

            qStdOut() << QStringLiteral("Error occured on reading remote files") <<endl ;
            return false;
        }

        if(!this->readRemote(this->getRoot(),QStringLiteral("/"))){ // top level files and folders
            qStdOut() << QStringLiteral("Error occured on reading remote files")<<endl  ;
            return false;
        }
    }



    this->driveJSONFileList.clear();

    qStdOut() << QStringLiteral("Reading local files and folders") <<endl ;

    if(!this->readLocal(this->workPath)){
        qStdOut() << QStringLiteral("Error occured on reading local files and folders") <<endl ;
        return false;
    }

    // make separately lists of objects
    QList<QString> keys = this->syncFileList.uniqueKeys();

    if((keys.size()>3) && (!this->getFlag(QStringLiteral("singleThread")))){// split list to few parts

        this->threadsRunning = new QSemaphore(3);

        QThread* t1 = new QThread(this);
        QThread* t2 = new QThread(this);
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


    }
    else{// sync as is

        this->checkFolderStructures();
        this->doSync(this->syncFileList);
    }

    //this->doSync(this->syncFileList);

    return true;
}

//=======================================================================================

MSFSObject::ObjectState MSGoogleDrive::filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote){



    if((local.exist)&&(remote.exist)){ //exists both files

        if(this->filterGoogleDocsMimeTypes(remote.extraData.find("mimeType").value().toString()) && this->getFlag(QStringLiteral("convertDoc"))){

            if(local.modifiedDate==remote.modifiedDate){
                return MSFSObject::ObjectState::Sync;
            }

        }

        if(local.md5Hash==remote.md5Hash){


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

//=======================================================================================

QHash<QString,MSFSObject> MSGoogleDrive::filelist_getFSObjectsByState(MSFSObject::ObjectState state){

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

QHash<QString,MSFSObject> MSGoogleDrive::filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state){

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

QHash<QString,MSFSObject> MSGoogleDrive::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type){

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

QHash<QString,MSFSObject> MSGoogleDrive::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type){

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

//===================================================================================

bool MSGoogleDrive::filelist_FSObjectHasParent(const MSFSObject &fsObject){

    //    if(fsObject.path==QStringLiteral("/")){
    //        return false;
    //    }
    //    else{
    //        return true;
    //    }

    return (fsObject.path.count(QStringLiteral("/"))>=1)&&(fsObject.path!=QStringLiteral("/"));
}

//=======================================================================================

MSFSObject MSGoogleDrive::filelist_getParentFSObject(const MSFSObject &fsObject){

    QString parentPath;

    if((fsObject.local.objectType==MSLocalFSObject::Type::file) || (fsObject.remote.objectType==MSRemoteFSObject::Type::file)){
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf(QStringLiteral("/")));
    }
    else{
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf(QStringLiteral("/")));
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


void MSGoogleDrive::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;

        object.value().remote.extraData.insert(QStringLiteral("id"), changedFSObject.remote.extraData.find(QStringLiteral("id")).value());
        object.value().remote.extraData.insert(QStringLiteral("mimeType"), changedFSObject.remote.extraData.find(QStringLiteral("mimeType")).value());
        object.value().remote.extraData.insert(QStringLiteral("exportLinks"), changedFSObject.remote.extraData.find(QStringLiteral("exportLinks")).value());

        //object.value().remote.data=changedFSObject.remote.data;
    }

}

//=======================================================================================

void MSGoogleDrive::doSync(QHash<QString, MSFSObject> fsObjectList){

    QHash<QString,MSFSObject>::iterator lf;

    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag(QStringLiteral("force"))){

        if(this->getOption(QStringLiteral("force"))==QStringLiteral("download")){

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

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced downloading.") )<<endl ;

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

                                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced uploading.") )<<endl ;

                                this->remote_file_update(&obj);
                            }
                        }
                        else{

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced uploading.") )<<endl ;

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

    qStdOut()<<QStringLiteral("Start syncronization") <<endl ;

    lf=fsObjectList.begin();

    for(;lf != fsObjectList.end();lf++){

        MSFSObject obj=lf.value();

        if((obj.state == MSFSObject::ObjectState::Sync)){

            continue;
        }

        switch((int)(obj.state)){

        case MSFSObject::ObjectState::ChangedLocal:

            qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Changed local. Uploading.") )<<endl ;

            this->remote_file_update(&obj);

            break;

        case MSFSObject::ObjectState::NewLocal:

            if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") )<<endl ;

                this->remote_file_insert(&obj);

            }
            else{

                if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") )<<endl ;

                    this->remote_file_insert(&obj);

                }
                else{

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete remote. Delete local.") )<<endl ;

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

            qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Changed remote. Downloading.") )<<endl ;

            this->remote_file_get(&obj);

            break;


        case MSFSObject::ObjectState::NewRemote:

            if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") )<<endl ;

                    this->remote_file_trash(&obj);

                }
                else{
                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") ) <<endl;

                    this->remote_file_get(&obj);
                }


            }
            else{

                if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") )<<endl ;

                    this->remote_file_trash(&obj);
                }
                else{

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") )<<endl ;

                    this->remote_file_get(&obj);
                }
            }

            break;


        case MSFSObject::ObjectState::DeleteLocal:

            if((obj.remote.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") ) <<endl;

                this->remote_file_get(&obj);

                break;
            }

            qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") ) <<endl;

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

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete remote. Deleting local.") ) <<endl;

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

    qStdOut()<<QStringLiteral("Syncronization end") <<endl ;
}


// ============= REMOTE FUNCTIONS BLOCK =============
//=======================================================================================

//bool MSGoogleDrive::remote_file_generateIDs(int count){

//    QList<QString> lst;

//    while(count > 0){
//        MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

//        req->setRequestUrl("https://www.googleapis.com/drive/v2/files/generateIds");
//        req->setMethod("get");

//        int c=0;

//        if(count<1000){
//            c=count;
//            count =0;
//        }
//        else{
//            c=1000;
//            count-=1000;
//        }

//        req->addQueryItem("maxResults",          QString::number(c));
//        req->addQueryItem("space",               "drive");
//        req->addQueryItem("access_token",           this->access_token);

//        req->exec();


//        if(!req->replyOK()){
//            req->printReplyError();
//            delete(req);
//            exit(1);
//        }

//        if(!this->testReplyBodyForError(req->readReplyText())){
//            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
//            exit(0);
//        }


//        QString content= req->readReplyText();

//        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
//        QJsonObject job = json.object();
//        QJsonArray v=job["ids"].toArray();

//        for(int i=0;i<v.size();i++){
//            lst.append(v[i].toString());
//        }

//        delete(req);

//    }

//    this->ids_list.setList(lst);

//    if(lst.size()>0){
//       return true;
//    }
//    else{
//        return false;
//    }

//}

//=======================================================================================
// download file from cloud
bool MSGoogleDrive::remote_file_get(MSFSObject* object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    if(object->remote.objectType == MSRemoteFSObject::Type::folder){

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.") <<endl ;
        return true;
    }


    QString id = object->remote.extraData.find(QStringLiteral("id")).value().toString();// object->remote.data["id"].toString();

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

afterReauth:

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files/")+id);
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);



    // document conversion support
    if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){

        req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files/")+id+"/export");


        QString ftype= object->fileName.right(object->fileName.size()- object->fileName.lastIndexOf(QStringLiteral("."))-1)  ;

        if(  object->remote.extraData.find(QStringLiteral("mimeType")).value().toString() == QStringLiteral("application/vnd.google-apps.spreadsheet")){

            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
            req->addQueryItem("mimeType", object->local.mimeType);

            if(/*(object->local.exist == false)&&*/(object->getObjectExtension().isEmpty())){
                object->fileName=object->fileName+QStringLiteral(".xlsx");
            }


        }

        if(object->remote.extraData.find(QStringLiteral("mimeType")).value().toString() ==QStringLiteral("application/vnd.google-apps.document")){

            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
            req->addQueryItem("mimeType", object->local.mimeType);

            if((!object->local.exist)&&(object->getObjectExtension().isEmpty())){
                object->fileName=object->fileName+QStringLiteral(".docx");
            }

        }

        if(object->remote.extraData.find(QStringLiteral("mimeType")).value().toString() == QStringLiteral("application/vnd.google-apps.presentation")){

            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.presentationml.presentation");
            req->addQueryItem("mimeType", object->local.mimeType);

            if((!object->local.exist)&&(object->getObjectExtension().isEmpty())){
                object->fileName=object->fileName+QStringLiteral(".pptx");
            }

        }

    }
    else{

        req->addQueryItem(QStringLiteral("alt"),   QStringLiteral("media"));
    }


    QString filePath=this->workPath+object->path + CCROSS_TMP_PREFIX + object->fileName;

    req->setOutputFile(filePath);
    req->exec();

    if(req->replyErrorText.contains(QStringLiteral("Unauthorized"))){
        delete(req);
        this->refreshToken();
        req = new MSHttpRequest(this->proxyServer);

        qStdOut() << QStringLiteral("GoogleDrive token expired. Refreshing token done. Retry last operation. ") ;

        goto afterReauth;
    }


    if(this->testReplyBodyForError(req->readReplyText())){

        this->local_actualizeTempFile(filePath);

        // document conversion support (change modified date to remote value)
//        if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){
            utimbuf tb;

            QString dd=(object->remote.extraData.find(QStringLiteral("modifiedDate")).value().toString());
            tb.actime=(this->toMilliseconds(dd,true)/1000);
            tb.modtime=(this->toMilliseconds(dd,true)/1000);

            filePath=this->workPath+object->path + object->fileName;

            utime(filePath.toLocal8Bit().constData(),&tb);
//        }

    }
    else{
        qStdOut() << QStringLiteral("Service error. ")<< this->getReplyErrorString(req->readReplyText())<< endl;
    }

    delete(req);
    return true;

}

//=======================================================================================

bool MSGoogleDrive::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.")  <<endl;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSFSObject po=this->filelist_getParentFSObject(*object);
    QString parentID="";
    if(po.path != ""){
        parentID= po.remote.extraData.find(QStringLiteral("id")).value().toString();//  po.remote.data["id"].toString();
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

afterReauth:

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/upload/drive/v2/files"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));
    req->addQueryItem(QStringLiteral("uploadType"),                     QStringLiteral("resumable"));

    // document conversion support
    if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){

        req->addQueryItem(QStringLiteral("convert"),                     QStringLiteral("true"));
        // correct mime type to ms office mime type

        QString ftype= object->fileName.right(object->fileName.size()- object->fileName.lastIndexOf(QStringLiteral("."))-1)  ;

        if(ftype==QStringLiteral("xlsx")){
            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        }

        if(ftype==QStringLiteral("xls")){
            object->local.mimeType=QStringLiteral("application/vnd.ms-excel");
        }

        if(ftype==QStringLiteral("docx")){
            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
        }

        if(ftype==QStringLiteral("doc")){
            object->local.mimeType=QStringLiteral("application/msword");
        }

        if(ftype==QStringLiteral("pptx")){
            object->local.mimeType=QStringLiteral("application/vnd.openxmlformats-officedocument.presentationml.presentation");
        }

        if(ftype==QStringLiteral("ppt")){
            object->local.mimeType=QStringLiteral("application/vnd.ms-powerpoint");
        }

        //        if(ftype=="odp"){
        //            object->local.mimeType="application/vnd.oasis.opendocument.presentation";
        //        }

        if(ftype==QStringLiteral("ods")){
            object->local.mimeType=QStringLiteral("application/vnd.oasis.opendocument.spreadsheet");
        }

        if(ftype==QStringLiteral("odt")){
            object->local.mimeType=QStringLiteral("application/vnd.oasis.opendocument.text");
        }

    }

    // collect request data body

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;

    if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){

        // remove file extension

        metaJson.insert(QStringLiteral("title"),object->fileName.left(object->fileName.lastIndexOf(".")));
    }
    else{
        metaJson.insert(QStringLiteral("title"),object->fileName);
    }


    QString rfcDate = this->toRFC3339(object->local.modifiedDate*1000);

    metaJson.insert(QStringLiteral("modifiedDate"),rfcDate);


    if(parentID != QStringLiteral("")){
        // create parents section
        QJsonArray parents;
        QJsonObject par;
        par.insert(QStringLiteral("id"),parentID);
        parents.append(par);

        metaJson.insert(QStringLiteral("parents"),parents);
    }

    metaData.append((QJsonDocument(metaJson).toJson()));

    // hack to avoid zerosize mime type (has effect in a fuse mode)
    if(object->local.mimeType.contains(QStringLiteral("zerosize"))){

        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(this->workPath+object->path+object->fileName);
        object->local.mimeType=type.name();
        object->local.fileSize = object->remote.fileSize;//QFileInfo().size();

        if(type.name().contains(QStringLiteral("text/"))){

            object->local.mimeType += QStringLiteral("; charset=utf-8");
        }
    }

    req->addHeader(QStringLiteral("X-Upload-Content-Type"),                      object->local.mimeType);

    QString filePath=this->workPath+object->path+object->fileName;

    // open a file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath <<endl ;
        delete(req);
        return false;
    }

    QMultiBuffer mb;
    mb.append(&file);
    mb.open(QIODevice::ReadOnly);

    req->addHeader(QStringLiteral("Content-Length"),QString::number(metaData.length()).toLocal8Bit());

    //    req->post(metaData);
    req->setInputDataStream(metaData);
    req->exec();

    if(!req->replyOK()){

        if(req->replyErrorText.contains(QStringLiteral("Unauthorized"))){
            delete(req);
            this->refreshToken();
            req = new MSHttpRequest(this->proxyServer);

            qStdOut() << QStringLiteral("GoogleDrive token expired. Refreshing token done. Retry last operation. ") <<endl;

            goto afterReauth;
        }

        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }


    QString uploadURI = req->getReplyHeader(QByteArray("Location"));//   header(QNetworkRequest::LocationHeader).toString();

    req->replyText.clear();


    // delete(req->query);
    delete(req);

    if( file.size() > GOOGLEDRIVE_CHUNK_SIZE){// multi-chunk upload

        quint64 cursorPosition=0;
        qint64 fSize = file.size();
        qint64 upsize=0;
        //quint64 passCount = (quint64)((double)fSize/(double)GOOGLEDRIVE_CHUNK_SIZE);

        do{

            req = new MSHttpRequest(this->proxyServer);
            req->setRequestUrl(uploadURI);

            //set upload block size

            quint64 blsz=0;

            if(cursorPosition == 0){

                blsz=GOOGLEDRIVE_CHUNK_SIZE;
            }
            else{

                if(fSize - cursorPosition < GOOGLEDRIVE_CHUNK_SIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=GOOGLEDRIVE_CHUNK_SIZE;
                }

            }

            //----------------------
#ifndef CCROSS_LIB
            //req->query = new QUrlQuery(req->url->query());// extract query string and setup his separately
#endif
            req->addHeader("Authorization",                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Length"),                    QString::number(blsz));
            req->addHeader(QStringLiteral("Content-Range"),                     QStringLiteral("bytes ")+QString::number(cursorPosition).toLocal8Bit()+QStringLiteral("-")+QString::number(cursorPosition+blsz-1).toLocal8Bit()+QStringLiteral("/")+QString::number(fSize).toLocal8Bit());
            req->addHeader(QStringLiteral("Content-Type"),                      object->local.mimeType);

            req->setMethod("put");

            //file.seek(cursorPosition);
            file.seek(cursorPosition);

            req->setInputDataStream(&file);
            req->exec();

            upsize+=blsz;

            //            req->put(&file);
            //            req->put(file.read(GOOGLEDRIVE_CHUNK_SIZE));

            if(!req->replyOK()){

                if(req->replyErrorText.contains(QStringLiteral("Unauthorized"))){
                    // delete(req->query);
                    delete(req);
                    this->refreshToken();
                    req = new MSHttpRequest(this->proxyServer);

                    qStdOut() << QStringLiteral("GoogleDrive token expired. Refreshing token done. Retry last operation. ")<<endl ;

                    goto afterReauth;
                }

                req->printReplyError();
                // delete(req->query);
                delete(req);
                return false;
            }

            cursorPosition +=  GOOGLEDRIVE_CHUNK_SIZE;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize){

                // delete(req->query);
                delete(req);
                break;
            }

            // delete(req->query);
            delete(req);

        }while (true);

    }
    else{// single-shot upload

        req = new MSHttpRequest(this->proxyServer);

afterReauth2:

        req->setRequestUrl(uploadURI);
#ifndef CCROSS_LIB
        //req->query = new QUrlQuery(req->url->query());// extract query string and setup his separately
#endif
        req->setMethod("put");

        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Type"),                      object->local.mimeType);
        req->addHeader(QStringLiteral("Content-Length"),                      QString::number(mb.size()));

        req->setInputDataStream(&mb);
        req->exec();

        //        req->put(&mb);
        //        req->put(file.readAll());

        file.close();

        if(!req->replyOK()){

            if(req->replyErrorText.contains(QStringLiteral("Unauthorized"))){
                // delete(req->query);
                delete(req);
                this->refreshToken();
                req = new MSHttpRequest(this->proxyServer);

                qStdOut() << QStringLiteral("GoogleDrive token expired. Refreshing token done. Retry last operation. ")<<endl ;

                goto afterReauth2;
            }

            req->printReplyError();
            // delete(req->query);
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
            // delete(req->query);
            delete(req);
            return false;
        }

        req->replyText.clear();
        ;


        // delete(req->query);
        delete(req);

    }

    return true;

}


//=======================================================================================

bool MSGoogleDrive::remote_file_update(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.") <<endl ;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    QString id = object->remote.extraData.find("id").value().toString();//  object->remote.data["id"].toString();

    MSFSObject po=this->filelist_getParentFSObject(*object);
    QString parentID="";
    if(po.path != ""){
        parentID = po.remote.extraData.find(QStringLiteral("id")).value().toString();//  po.remote.data["id"].toString();
    }


    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/upload/drive/v2/files/")+id);
    req->setMethod(QStringLiteral("put"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));
    req->addQueryItem(QStringLiteral("uploadType"),                     QStringLiteral("resumable"));

    if(this->getFlag(QStringLiteral("noNewRev"))){
        req->addQueryItem(QStringLiteral("newRevision"),                     QStringLiteral("false"));
        req->addQueryItem(QStringLiteral("pinned"),                          QStringLiteral("true"));
    }

    req->addQueryItem(QStringLiteral("setModifiedDate"),                     QStringLiteral("true"));

    // collect request data body

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;

    if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){

        // remove file extension

        metaJson.insert(QStringLiteral("title"),object->fileName.left(object->fileName.lastIndexOf(".")));
    }
    else{
        metaJson.insert(QStringLiteral("title"),object->fileName);
    }

    QString rfcDate = this->toRFC3339(object->local.modifiedDate*1000);

    metaJson.insert(QStringLiteral("modifiedDate"),rfcDate);

    if(parentID != QStringLiteral("")){
        // create parents section
        QJsonArray parents;
        QJsonObject par;
        par.insert(QStringLiteral("id"),parentID);
        parents.append(par);

        metaJson.insert(QStringLiteral("parents"),parents);
    }

    metaData.append((QJsonDocument(metaJson).toJson()));

    req->addHeader(QStringLiteral("X-Upload-Content-Type"), object->local.mimeType);

    QString filePath=this->workPath+object->path+object->fileName;

    if(object->isDocFormat && this->getFlag(QStringLiteral("convertDoc"))){
        filePath+=QStringLiteral(".")+this->getOfficeFileExtensionByMimeType(object->remote.extraData.find("mimeType").value().toString());
    }


    // read file content and put him into request body
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath <<endl  ;
        delete(req);
        return false;
    }

    QMultiBuffer mb;
    mb.append(&file);
    mb.open(QIODevice::ReadOnly);

    req->addHeader(QStringLiteral("Content-Length"),QString::number(metaData.length()).toLocal8Bit());

    req->setInputDataStream(metaData);
    req->exec();
    //    req->put(metaData);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }

    QString uploadURI = req->getReplyHeader(QByteArray("Location"));//   header(QNetworkRequest::LocationHeader).toString();

    delete(req);


    if( file.size() > GOOGLEDRIVE_CHUNK_SIZE){// multi-chunk upload

        quint64 cursorPosition=0;
        qint64 fSize=file.size();
        //quint64 passCount=(quint64)((double)fSize/(double)GOOGLEDRIVE_CHUNK_SIZE);

        do{

            req = new MSHttpRequest(this->proxyServer);
            req->setRequestUrl(uploadURI);

            //set upload block size

            quint64 blsz=0;

            if(cursorPosition == 0){

                blsz=GOOGLEDRIVE_CHUNK_SIZE;
            }
            else{

                if(fSize - cursorPosition < GOOGLEDRIVE_CHUNK_SIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=GOOGLEDRIVE_CHUNK_SIZE;
                }

            }

            //----------------------
#ifndef CCROSS_LIB
            //req->query = new QUrlQuery(req->url->query());// extract query string and setup his separately
#endif

            QString tst = QStringLiteral("bytes ")+QString::number(cursorPosition).toLocal8Bit()+QStringLiteral("-")+QString::number(cursorPosition+blsz-1).toLocal8Bit()+QStringLiteral("/")+QString::number(fSize).toLocal8Bit();

            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Length"),                    QString::number(blsz));
            req->addHeader(QStringLiteral("Content-Range"),                     QStringLiteral("bytes ")+QString::number(cursorPosition).toLocal8Bit()+"-"+QString::number(cursorPosition+blsz-1).toLocal8Bit()+"/"+QString::number(fSize).toLocal8Bit());

            file.seek(cursorPosition);

            req->put(file.read(GOOGLEDRIVE_CHUNK_SIZE));

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            cursorPosition +=  GOOGLEDRIVE_CHUNK_SIZE;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize){

                delete(req);
                break;
            }

            delete(req);

        }while (true);

    }
    else{// single-shot upload

        req = new MSHttpRequest(this->proxyServer);

        req->setRequestUrl(uploadURI);
#ifndef CCROSS_LIB
        //req->query = new QUrlQuery(req->url->query());// extract query string and setup his separately
#endif
        req->setMethod("put");

        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Type"),                      object->local.mimeType);
        req->addHeader(QStringLiteral("Content-Length"),                      QString::number(file.size()));

        req->setInputDataStream(&mb);
        req->exec();


        file.close();

        if(!req->replyOK()){
            req->printReplyError();
            // delete(req->query);
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
            // delete(req->query);
            delete(req);
            return false;
        }

        // delete(req->query);
        delete(req);

    }

    return true;


}

//=======================================================================================

bool MSGoogleDrive::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;
    metaJson.insert(QStringLiteral("title"),object->fileName);
    metaJson.insert(QStringLiteral("mimeType"),QStringLiteral("application/vnd.google-apps.folder"));

    //metaData.append(QString(QJsonDocument(metaJson).toJson()).toLocal8Bit());
    metaData.append((QJsonDocument(metaJson).toJson()));

    //    req->post(metaData);

    req->setInputDataStream(metaData);
    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }



    QString filePath=this->workPath+object->path+object->fileName;

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    object->local.newRemoteID=job[QStringLiteral("id")].toString();

    object->remote.extraData.insert(QStringLiteral("id"),job[QStringLiteral("id")].toString());
    object->remote.extraData.insert(QStringLiteral("mimeType"),job[QStringLiteral("mimeType")].toString());
    object->remote.extraData.insert(QStringLiteral("exportLinks"),job[QStringLiteral("exportLinks")].toObject());

    //object->remote.data=job;
    object->remote.exist=true;

    if(job["id"].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when folder create ")+filePath+QStringLiteral(" on remote") <<endl ;
        delete(req);
        return false;
    }

    delete(req);

    this->filelist_populateChanges(*object);
    return true;

}

//=======================================================================================

bool MSGoogleDrive::remote_file_makeFolder(MSFSObject *object, const QString &parentID){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;
    metaJson.insert(QStringLiteral("title"),object->fileName);
    metaJson.insert(QStringLiteral("mimeType"),QStringLiteral("application/vnd.google-apps.folder"));

    if(parentID != ""){
        // create parents section
        QJsonArray parents;
        QJsonObject par;
        par.insert(QStringLiteral("id"),parentID);
        parents.append(par);

        metaJson.insert(QStringLiteral("parents"),parents);
    }

    //metaData.append(QString(QJsonDocument(metaJson).toJson()).toLocal8Bit());
    metaData.append((QJsonDocument(metaJson).toJson()));

    //    req->post(metaData);
    req->setInputDataStream(metaData);
    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }



    QString filePath=this->workPath+object->path+object->fileName;

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    object->local.newRemoteID=job[QStringLiteral("id")].toString();

    object->remote.extraData.insert(QStringLiteral("id"),job[QStringLiteral("id")].toString());
    object->remote.extraData.insert(QStringLiteral("mimeType"),job[QStringLiteral("mimeType")].toString());
    object->remote.extraData.insert(QStringLiteral("exportLinks"),job[QStringLiteral("exportLinks")].toObject());

    // object->remote.data=job;
    object->remote.exist=true;

    if(job["id"].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when folder create ")+filePath+QStringLiteral(" on remote") <<endl ;
        delete(req);
        return false;
    }

    delete(req);

    this->filelist_populateChanges(*object);
    return true;

}

//=======================================================================================

bool MSGoogleDrive::remote_file_trash(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    QString id = object->remote.extraData.find(QStringLiteral("id")).value().toString();//   object->remote.data["id"].toString();

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files/")+id+"/trash");
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return false;
    }


    QString content=req->readReplyText();

    QString filePath=this->workPath+object->path+object->fileName;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    if(job[QStringLiteral("id")].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when move to trash ")+filePath+QStringLiteral(" on remote")  <<endl;
    }

    delete(req);
    return true;

}

//=======================================================================================

bool MSGoogleDrive::remote_createDirectory(const QString &path){

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

                MSFSObject parObj=this->filelist_getParentFSObject(f.value());
                //                this->remote_file_makeFolder(&f.value(),parObj.remote.data["id"].toString());
                this->remote_file_makeFolder(&f.value(),parObj.remote.extraData.find(QStringLiteral("id")).value().toString());

            }
            else{

                this->remote_file_makeFolder(&f.value());
            }
        }
        //        else{// if this path not exists on remote and not listed as local because include/exclude filter working

        //            currPath=currPath+"/"+dirs[i];

        //            if(this->filelist_FSObjectHasParent(currPath)){

        //            }
        //            else{
        //                this->remote_file_makeFolder();
        //            }

        //        }
    }
    return true;
}



// ============= LOCAL FUNCTIONS BLOCK =============
//=======================================================================================

void MSGoogleDrive::local_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================

void MSGoogleDrive::local_removeFile(const QString &path){

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
        //tfs.mkdir(this->workPath + tfi.absolutePath().replace(this->workPath,QStringLiteral("")));
        this->createDirectoryPath(this->workPath + tfi.absolutePath().replace(this->workPath,QStringLiteral("")));
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

//=======================================================================================

void MSGoogleDrive::local_removeFolder(const QString &path){

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
        this->createDirectoryPath(this->workPath + tfi.absolutePath().replace(this->workPath,QStringLiteral("")));
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


bool MSGoogleDrive::testReplyBodyForError(const QString &body){// return true if reply is ok

    if(body.contains(QStringLiteral("\"error\": {"))){
        QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
        QJsonObject job = json.object();

        QJsonValue e=(job[QStringLiteral("error")].toObject())[QStringLiteral("code")];
        if(e.isNull()){
            return true;
        }
        else{

            int code=e.toInt(0);
            return code == 0;
        }

    }
    else{
        return true;
    }
}


QString MSGoogleDrive::getReplyErrorString(const QString &body){

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job[QStringLiteral("error")].toObject())[QStringLiteral("message")];

    return e.toString();

}




bool MSGoogleDrive::directUpload(const QString &url, const QString &remotePath){

    qStdOut() << "NOT INPLEMENTED YET"<<endl;
    return false;

    // get remote filelist

    QString remoteFilePath = remotePath;
    this->createHashFromRemote();

    this->readRemote(this->getRoot(),QStringLiteral("/"));// top level files and folders



    // download file into temp file ---------------------------------------------------------------

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    QString tempFileName=this->generateRandom(10);
    QString filePath=this->workPath+QStringLiteral("/")+tempFileName;

    req->setMethod(QStringLiteral("get"));
    req->download(url,filePath);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }

    QFileInfo fileRemote(remotePath);
    QString path=fileRemote.absoluteFilePath();

    path=QString(path).left(path.lastIndexOf(QStringLiteral("/")));

    QString targetFileName=remoteFilePath.replace(path,QStringLiteral("")).replace(QStringLiteral("/"),QStringLiteral(""));

    QString cPath=QStringLiteral("/");

    if(path!=QStringLiteral("")){

        QStringList dirs=path.split(QStringLiteral("/"));

        MSFSObject* obj=0;

        QHash<QString,MSFSObject>::iterator ii;

        for(int i=0;i<dirs.size();i++){


            ii=this->syncFileList.find(cPath+dirs[i]);



            if(ii==this->syncFileList.end()){// if object exists in Google Drive

                obj=new MSFSObject();
                obj->path=cPath;
                obj->fileName=dirs[i];
                obj->state=MSFSObject::ObjectState::NewLocal;
                obj->local.objectType=  MSLocalFSObject::Type::folder;
                obj->remote.exist=false;

                this->syncFileList.insert(obj->path+obj->fileName,*obj);

                delete(obj);

                i--;
                continue;
            }


            //            obj=new MSFSObject();
            //            obj->path="/"+dirs[i];
            //            obj->fileName="";
            //            obj->remote.exist=false;
            //            this->remote_file_makeFolder(obj);
            //            delete(obj);

            if(dirs[i] != QStringLiteral("")){
                cPath+=dirs[i]+QStringLiteral("/");
            }

        }


    }



    QHash<QString,MSFSObject>::iterator lf;

    QHash<QString,MSFSObject> localFolders=this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::folder);
    localFolders=this->filelist_getFSObjectsByState(localFolders,MSFSObject::ObjectState::NewLocal);

    lf=localFolders.begin();

    while(lf != localFolders.end()){

        if(lf.value().path == "/"){
            lf++;
            continue;
        }

        this->remote_createDirectory(lf.key());

        lf++;
    }


    delete(req);


    // upload file to remote ------------------------------------------------------------------------


    MSFSObject *object=0;
    object=new MSFSObject();
    object->path=path+QStringLiteral("/");
    object->fileName=tempFileName;
    object->getLocalMimeType(this->workPath+QStringLiteral("/")+tempFileName);

    cPath=cPath.left(cPath.lastIndexOf(QStringLiteral("/")));

    QString bound=QStringLiteral("ccross-data");
    MSFSObject po;
    QString parentID="";
    if(cPath !=""){
        po=this->syncFileList.find(cPath).value();
    }

    if(po.path != ""){
        parentID = po.remote.extraData.find(QStringLiteral("id")).value().toString();//   po.remote.data["id"].toString();
    }

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/upload/drive/v2/files"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("multipart/related; boundary=")+QString(bound).toLocal8Bit());
    req->addQueryItem(QStringLiteral("uploadType"),                     QStringLiteral("multipart"));

    // collect request data body

    QByteArray metaData;
    metaData.append(QString(QStringLiteral("--")+bound+QStringLiteral("\r\n")).toLocal8Bit());
    metaData.append(QString(QStringLiteral("Content-Type: application/json; charset=UTF-8\r\n\r\n")).toLocal8Bit());

    //make file metadata in json representation
    QJsonObject metaJson;


    metaJson.insert(QStringLiteral("title"),targetFileName);


    if(parentID != ""){
        // create parents section
        QJsonArray parents;
        QJsonObject par;
        par.insert(QStringLiteral("id"),parentID);
        parents.append(par);

        metaJson.insert(QStringLiteral("parents"),parents);
    }

    //metaData.append(QString(QJsonDocument(metaJson).toJson()).toLocal8Bit());
    metaData.append((QJsonDocument(metaJson).toJson()));

    metaData.append(QString(QStringLiteral("\r\n--")+bound+QStringLiteral("\r\n")).toLocal8Bit());

    QByteArray mediaData;

    mediaData.append(QString(QStringLiteral("Content-Type: ")+object->local.mimeType+QStringLiteral("\r\n\r\n")).toLocal8Bit());

    filePath=this->workPath+QStringLiteral("/")+tempFileName;

    // read file content and put him into request body
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath  ;
        delete(req);
        return false;
    }
    mediaData.append(file.readAll());
    file.close();

    file.remove();


    mediaData.append(QString(QStringLiteral("\r\n--")+bound+QStringLiteral("--")).toLocal8Bit());

    req->addHeader(QStringLiteral("Content-Length"),QString::number(metaData.length()+mediaData.length()).toLocal8Bit());

    req->setInputDataStream(metaData+mediaData);
    req->exec();
    //    req->post(metaData+mediaData);



    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;

        exit(0);
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    object->local.newRemoteID=job[QStringLiteral("id")].toString();

    if(job[QStringLiteral("id")].toString()==""){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")  ;
    }

    delete(req);
    delete(object);

    return true;
}




QString MSGoogleDrive::getInfo(){

    //    "storageQuota": {
    //        "limit": long,
    //        "usage": long,
    //        "usageInDrive": long,
    //        "usageInDriveTrash": long
    //      }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v3/about"));
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);

    req->addQueryItem(QStringLiteral("fields"),     QStringLiteral("storageQuota,user"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return QStringLiteral("false");
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return QStringLiteral("false");
    }


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    //int zz=job["storageQuota"].toObject().size();

    if(job[QStringLiteral("storageQuota")].toObject().size()== 0){
        //qStdOut()<< "Error getting cloud information "  ;
        return QStringLiteral("false");
    }

    delete(req);

    QJsonObject out;
    out[QStringLiteral("account")]=job[QStringLiteral("user")].toObject()[QStringLiteral("emailAddress")].toString();
    out[QStringLiteral("total")]=job[QStringLiteral("storageQuota")].toObject()[QStringLiteral("limit")].toString();
    out[QStringLiteral("usage")]=job[QStringLiteral("storageQuota")].toObject()[QStringLiteral("usage")].toString();

    return QString( QJsonDocument(out).toJson());
}







bool MSGoogleDrive::remote_file_empty_trash(){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://www.googleapis.com/drive/v2/files/trash"));
    req->setMethod(QStringLiteral("delete"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    qStdOut() << endl;
    qStdOut() << "Trash bin of cloud was been cleared"<<endl;
    delete(req);
    return true;

}
