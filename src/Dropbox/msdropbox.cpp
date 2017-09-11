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

#include "msdropbox.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


//  150000000

MSDropbox::MSDropbox() :
    MSCloudProvider()
{
    this->providerName=     "Dropbox";
    this->tokenFileName=    ".dbox";
    this->stateFileName=    ".dbox_state";
    this->trashFileName=    ".trash_dbox";

}


//=======================================================================================

bool MSDropbox::auth(){

    connect(this,SIGNAL(oAuthCodeRecived(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));
    connect(this,SIGNAL(oAuthError(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));


    MSRequest* req=new MSRequest(this->proxyServer);

    //www.dropbox.com/1/oauth2/authorize?client_id=jqw8z760uh31l92&response_type=code&state=hGDSAGDKJASHDKJHASLKFH

    req->setRequestUrl("https://www.dropbox.com/1/oauth2/authorize");
    req->setMethod("get");

    req->addQueryItem("redirect_uri",           "http://127.0.0.1:1973");
    req->addQueryItem("response_type",          "code");
    req->addQueryItem("client_id",              "jqw8z760uh31l92");
    req->addQueryItem("state",                  this->generateRandom(20));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    this->startListener(1973);

    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<< tr("Please go to this URL and confirm application credentials\n")<<endl;

    qStdOut() << req->replyURL;
    qStdOut()<<""<<endl;


    delete(req);


    QEventLoop loop;
    connect(this, SIGNAL(providerAuthComplete()), &loop, SLOT(quit()));
    loop.exec();


    return true;

}

//=======================================================================================


bool MSDropbox::onAuthFinished(const QString &html, MSCloudProvider *provider){

    Q_UNUSED(provider)

    MSRequest* req=new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/1/oauth2/token");
    req->setMethod("post");

    req->addQueryItem("client_id",          "jqw8z760uh31l92");
    req->addQueryItem("client_secret",      "eeuwcu3kzqrbl9j");
    req->addQueryItem("code",               html.trimmed());
    req->addQueryItem("grant_type",         "authorization_code");
    req->addQueryItem("redirect_uri",           "http://127.0.0.1:1973");

    req->exec();


    if(!req->replyOK()){
        delete(req);
        req->printReplyError();
        this->providerAuthStatus=false;
        emit providerAuthComplete();
        return false;
    }

//    if(!this->testReplyBodyForError(req->readReplyText())){
//        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
//        exit(0);
//    }


    QString content= req->replyText;//lastReply->readAll();

    //qStdOut() << content;


    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job["access_token"].toString();

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

//=======================================================================================

void MSDropbox::saveTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"access_token\" : \""+this->token+"\"}";
    key.close();

}


//=======================================================================================

bool MSDropbox::loadTokenFile(const QString &path){

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

    key.close();
    return true;
}


//=======================================================================================

void MSDropbox::loadStateFile(){

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

void MSDropbox::saveStateFile(){


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

bool MSDropbox::refreshToken(){

    this->access_token=this->token;
    return true;
}

//=======================================================================================

bool MSDropbox::createHashFromRemote(){

    if(!this->readRemote()){

        return false;

    }

    MSRequest* req=new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/2/files/list_folder");
    req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));




    req->exec();


    if(!req->replyOK()){
        delete(req);
        req->printReplyError();
        return false;
    }



    QString list=req->readReplyText();

    delete(req);
    return true;
}


//=======================================================================================

bool MSDropbox::readRemote(){ //QString parentId,QString currentPath


    MSRequest* req=new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/2/files/list_folder");
    req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));


    QJsonObject d;

    d.insert("path",QString(""));
    d.insert("recursive",true);
    d.insert("include_media_info",false);
    d.insert("include_deleted",false);


    QByteArray metaData;

    metaData.append((QJsonDocument(d).toJson()));

    req->addHeader("Content-Length",QString::number(metaData.length()).toLocal8Bit());

    req->post(metaData);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString list=req->readReplyText();

    delete(req);

    QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = json.object();
    QJsonArray entries= job["entries"].toArray();

    QFileInfo fi=QFileInfo(entries[0].toObject()["path_display"].toString()) ;

    bool hasMore=(job["has_more"].toBool());

    do{


        for(int i=0;i<entries.size();i++){

                    MSFSObject fsObject;

                    QJsonObject o=entries[i].toObject();

                    fsObject.path = QFileInfo(o["path_display"].toString()).path()+"/";

                    if(fsObject.path == "//"){
                       fsObject.path ="/";
                    }

                    fsObject.remote.fileSize=  o["size"].toInt();
                    fsObject.remote.data=o;
                    fsObject.remote.exist=true;
                    fsObject.isDocFormat=false;

                    fsObject.state=MSFSObject::ObjectState::NewRemote;

                    if(this->isFile(o)){

                        fsObject.fileName=o["name"].toString();
                        fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                        fsObject.remote.modifiedDate=this->toMilliseconds(o["client_modified"].toString(),true);
                     }

                     if(this->isFolder(o)){

                         fsObject.fileName=o["name"].toString();
                         fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                         fsObject.remote.modifiedDate=this->toMilliseconds(o["client_modified"].toString(),true);

                      }

                      if(! this->filterServiceFileNames(o["path_display"].toString())){// skip service files and dirs

                          continue;
                      }


                      if(this->getFlag("useInclude") && this->includeList != ""){//  --use-include

                          if(this->filterIncludeFileNames(o["path_display"].toString())){
                              i++;
                              continue;
                          }
                      }
                      else{// use exclude by default

                          if(this->excludeList != ""){
                              if(!this->filterExcludeFileNames(o["path_display"].toString())){
                                  i++;
                                  continue;
                              }
                          }
                      }


                      this->syncFileList.insert(o["path_display"].toString(), fsObject);

        }

        hasMore=(job["has_more"].toBool());

        if(hasMore){

            MSRequest* req=new MSRequest(this->proxyServer);

            req->setRequestUrl("https://api.dropboxapi.com/2/files/list_folder/continue");
            req->setMethod("post");

            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

            QJsonObject d;

            d.insert("cursor",job["cursor"].toString());

            QByteArray metaData;

            metaData.append(QString(QJsonDocument(d).toJson()).toLocal8Bit());

            req->addHeader("Content-Length",QString::number(metaData.length()).toLocal8Bit());

            req->post(metaData);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }



            QString list=req->readReplyText();

            delete(req);

            json = QJsonDocument::fromJson(list.toUtf8());
            job = json.object();
            entries= job["entries"].toArray();

            hasMore=true;//(job["has_more"].toBool());

        }



    }while(hasMore==true);


    return true;
}

bool MSDropbox::_readRemote(const QString &rootPath){

    Q_UNUSED(rootPath);

    return this->readRemote();

}



//=======================================================================================


bool MSDropbox::isFile(const QJsonValue &remoteObject){
    if(remoteObject.toObject()[".tag"].toString()=="file"){
        return true;
    }
    return false;
}

//=======================================================================================

bool MSDropbox::isFolder(const QJsonValue &remoteObject){
    if(remoteObject.toObject()[".tag"].toString()=="folder"){
        return true;
    }
    return false;
}

//=======================================================================================

bool MSDropbox::createSyncFileList(){

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



    if(!this->readRemote()){// top level files and folders

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
 //this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));


    // make separately lists of objects
    QList<QString> keys = this->syncFileList.uniqueKeys();

    if(keys.size()>3){// split list to few parts

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


//        QHash<QString,MSFSObject> L1;
//        QHash<QString,MSFSObject> L2;
//        QHash<QString,MSFSObject> L3;

        for(int i=0;i<keys.size();i+=3){

            if(i<=keys.size()-1){
                thr1->threadSyncList.insert(keys[i],this->syncFileList.find(keys[i]).value());
            }
            if(i+1 <= keys.size()-1){
                thr2->threadSyncList.insert(keys[i+1],this->syncFileList.find(keys[i+1]).value());
            }
            if(i+2 <= keys.size()-1){
                thr3->threadSyncList.insert(keys[i+2],this->syncFileList.find(keys[i+2]).value());
            }

        }
        //this->doSync(L2);

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

//=======================================================================================


bool MSDropbox::readLocal(const QString &path){



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



            if(i!=this->syncFileList.end()){// if object exists in a cloud

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                //fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
                fsObject->local.exist=true;
                fsObject->getLocalMimeType(this->workPath);

                if(fi.isDir()){
                    fsObject->local.objectType=MSLocalFSObject::Type::folder;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000 * 1000;// /1000 added 20.08.17 *1000 added 3.09.17
                }
                else{

                    fsObject->local.objectType=MSLocalFSObject::Type::file;
                    fsObject->local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000 * 1000;// /1000 added 20.08.17 *1000 added 3.09.17

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
                //fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
                fsObject.local.exist=true;

                if(fi.isDir()){
                    fsObject.local.objectType=MSLocalFSObject::Type::folder;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000 * 1000;// /1000 added 20.08.17 *1000 added 3.09.17
                }
                else{

                    fsObject.local.objectType=MSLocalFSObject::Type::file;
                    fsObject.local.modifiedDate=this->toMilliseconds(fi.lastModified(),true)/1000 * 1000;// /1000 added 20.08.17; *1000 added 3.09.17

                }

                fsObject.state=this->filelist_defineObjectState(fsObject.local,fsObject.remote);

                fsObject.isDocFormat=false;


                this->syncFileList.insert(relPath,fsObject);

            }

        }

        return true;
}


//=======================================================================================


bool MSDropbox::readLocalSingle(const QString &path){

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

            if(i!=this->syncFileList.end()){// if object exists in a cloud

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                //fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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
                //fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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


MSFSObject::ObjectState MSDropbox::filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote){


    if((local.exist)&&(remote.exist)){ //exists both files

//        if(local.md5Hash==remote.md5Hash){


//                return MSFSObject::ObjectState::Sync;

//        }
//        else{

            // compare last modified date for local and remote
            if(local.modifiedDate == remote.modifiedDate){

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


void MSDropbox::checkFolderStructures(){

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



}

//=======================================================================================


void MSDropbox::doSync(QHash<QString, MSFSObject> fsObjectList){

    QHash<QString,MSFSObject>::iterator lf;

//    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

//        // create new folder structure on remote

//        qStdOut()<<"Checking folder structure on remote" <<endl;

//        QHash<QString,MSFSObject> localFolders=this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::folder);
//        localFolders=this->filelist_getFSObjectsByState(localFolders,MSFSObject::ObjectState::NewLocal);

//        lf=localFolders.begin();

//        while(lf != localFolders.end()){

//            this->remote_createDirectory(lf.key());

//            lf++;
//        }
//    }
//    else{

//        // create new folder structure on local

//        qStdOut()<<"Checking folder structure on local" <<endl;

//        QHash<QString,MSFSObject> remoteFolders=this->filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type::folder);
//        remoteFolders=this->filelist_getFSObjectsByState(remoteFolders,MSFSObject::ObjectState::NewRemote);

//        lf=remoteFolders.begin();

//        while(lf != remoteFolders.end()){

//            this->local_createDirectory(this->workPath+lf.key());

//            lf++;
//        }

//        // trash local folder
//        QHash<QString,MSFSObject> trashFolders=this->filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type::folder);
//        trashFolders=this->filelist_getFSObjectsByState(trashFolders,MSFSObject::ObjectState::DeleteRemote);

//        lf=trashFolders.begin();

//        while(lf != trashFolders.end()){


//            this->local_removeFolder(lf.key());

//            lf++;
//        }

//    }




    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag("force")){

        if(this->getOption("force")=="download"){

            qStdOut()<<"Start downloading in force mode" <<endl;

            lf=fsObjectList.begin();

            for(;lf != fsObjectList.end();lf++){

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

    lf=fsObjectList.begin();

    for(;lf != fsObjectList.end();lf++){

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


//bool MSDropbox::remote_file_generateIDs(int count) {
//// absolete

//    // fix warning message
//    count++;
//    return false;
//}

QHash<QString,MSFSObject>   MSDropbox::filelist_getFSObjectsByState(MSFSObject::ObjectState state) {

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


QHash<QString,MSFSObject>   MSDropbox::filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state) {

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


QHash<QString,MSFSObject>   MSDropbox::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type){


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


QHash<QString,MSFSObject>   MSDropbox::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type){

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

bool MSDropbox::filelist_FSObjectHasParent(const MSFSObject &fsObject){
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



MSFSObject MSDropbox::filelist_getParentFSObject(const MSFSObject &fsObject){

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


void MSDropbox::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}



bool MSDropbox::testReplyBodyForError(const QString &body) {

    if(body.contains("\"error\": {")){

        return false;

    }
    else{
        return true;
    }

}


QString MSDropbox::getReplyErrorString(const QString &body) {

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job["error_summary"]);

    return e.toString();

}



//=======================================================================================

// download file from cloud
bool MSDropbox::remote_file_get(MSFSObject* object){

    if(this->getFlag("dryRun")){
        return true;
    }

    QString id=object->remote.data["id"].toString();

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://content.dropboxapi.com/2/files/download");
    req->setMethod("post");

    req->addHeader("Authorization","Bearer "+this->access_token);
    req->addHeader("Dropbox-API-Arg","{\"path\":\""+id+"\"}");
    req->addHeader("Content-Type",QByteArray()); // this line needed for requests with libcurl version of MSRequest

    QString filePath=this->workPath+object->path+object->fileName;

    QByteArray ba;
#ifdef CCROSS_LIB
    req->post(ba);
#endif
#ifndef CCROSS_LIB
    req->syncDownloadWithPost(filePath,ba);
#endif



    if(this->testReplyBodyForError(req->readReplyText())){

        if(object->remote.objectType==MSRemoteFSObject::Type::file){

#ifdef CCROSS_LIB
            this->local_writeFileContent(filePath,req);
#endif
            // set remote "change time" for local file

            utimbuf tb;
            tb.actime=(this->toMilliseconds(object->remote.data["client_modified"].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(object->remote.data["client_modified"].toString(),true))/1000;;

            utime(filePath.toStdString().c_str(),&tb);
        }
    }
    else{

        if(! this->getReplyErrorString(req->readReplyText()).contains( "path/not_file/")){
            qStdOut() << "Service error. "<< this->getReplyErrorString(req->readReplyText());
            delete(req);
            return false;
        }
    }


    delete(req);
    return true;

}

//=======================================================================================

bool MSDropbox::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return true;
    }

    if(this->getFlag("dryRun")){
        return true;
    }


    // start session ===========

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/start");
   // req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
//    unsigned int passCount=(unsigned int)((unsigned int)fSize / (unsigned int)DROPBOX_CHUNK_SIZE);// count of 150MB blocks


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }

    req->post(mediaData);


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job["session_id"].toString();

    if(sessID==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
        delete(req);
        return false;
    }

    delete(req);

    req=new MSRequest(this->proxyServer);

    if( file.size() <= DROPBOX_CHUNK_SIZE){// onepass and finalize uploading

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert("session_id",sessID);
            cursor.insert("offset",0);
            QJsonObject commit;
            commit.insert("path",object->path+object->fileName);
            commit.insert("mode",QString("add"));
            commit.insert("autorename",false);
            commit.insert("mute",false);
            commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert("cursor",cursor);
            doc.insert("commit",commit);

            req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
//            QByteArray ba;

//            ba.append(file.read(fSize));


            req->addHeader("Content-Length",QString::number(file.size()).toLocal8Bit());

            req->post(file.read(fSize));

            file.close();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                return false;
            }

            //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
            // This code was added for FUSE

            QString content = req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();

            utimbuf tb;
            tb.actime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;

            utime(filePath.toStdString().c_str(),&tb);
            //------------------------------------------------------------------------------------------------------------------


    }
    else{ // multipass uploading

        int cursorPosition=0;
        int blsz=0;
        qint64 fSize = file.size();

        //for(int i=0;i<passCount;i++){
        do{

            if(cursorPosition == 0){

                blsz=DROPBOX_CHUNK_SIZE;
            }
            else{

                if(fSize - cursorPosition < DROPBOX_CHUNK_SIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=DROPBOX_CHUNK_SIZE;
                }

            }

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/append");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert("session_id",sessID);
            cursor1->insert("offset",cursorPosition);


            req->addHeader("Dropbox-API-Arg",QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            //QByteArray* ba1=new QByteArray();

            file.seek(cursorPosition);
            //ba1->append(file.read(DROPBOX_CHUNK_SIZE));



            req->addHeader("Content-Length",QString::number(blsz).toLocal8Bit());
            req->post(file.read(blsz));

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

            delete(req);

            delete(cursor1);
            cursorPosition += blsz;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize - DROPBOX_CHUNK_SIZE){

                req=new MSRequest(this->proxyServer);
                //cursorPosition -= blsz;
                break;
            }

            req=new MSRequest(this->proxyServer);
        }while(true);

        // finalize upload

        req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Type",                      QString("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert("session_id",sessID);
        cursor.insert("offset",cursorPosition);
        QJsonObject commit;
        commit.insert("path",object->path+object->fileName);
        commit.insert("mode",QString("add"));
        commit.insert("autorename",false);
        commit.insert("mute",false);
        commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert("cursor",cursor);
        doc.insert("commit",commit);

        req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
        //QByteArray ba;

        file.seek(cursorPosition);
        //ba.append(file.read(fSize-cursorPosition));


        req->addHeader("Content-Length",QString::number(fSize-cursorPosition).toLocal8Bit());
        req->post(file.read(fSize-cursorPosition));

        file.close();

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
            return false;
        }

        //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
        // This code was added for FUSE

        QString content = req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();

        utimbuf tb;
        tb.actime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;
        tb.modtime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;

        utime(filePath.toStdString().c_str(),&tb);
        //------------------------------------------------------------------------------------------------------------------


    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob["id"].toString();

    if(rjob["id"].toString()==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
        return false;
    }

    delete(req);
    return true;
}


//=======================================================================================

bool MSDropbox::remote_file_update(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return true;
    }

    if(this->getFlag("dryRun")){
        return true;
    }


    // start session ===========

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/start");
   // req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    //int passCount=fSize/DROPBOX_CHUNK_SIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }

    req->post(mediaData);


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job["session_id"].toString();

    if(sessID==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
        delete(req);
        return false;
    }

    delete(req);

    req=new MSRequest(this->proxyServer);

    if(file.size() <= DROPBOX_CHUNK_SIZE){// onepass and finalize uploading

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert("session_id",sessID);
            cursor.insert("offset",0);
            QJsonObject commit;
            commit.insert("path",object->path+object->fileName);
            commit.insert("mode",QString("overwrite"));
            commit.insert("autorename",false);
            commit.insert("mute",false);
            commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert("cursor",cursor);
            doc.insert("commit",commit);

            req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
//            QByteArray ba;

//            ba.append(file.read(fSize));


            req->addHeader("Content-Length",QString::number(file.size()).toLocal8Bit());
            req->post(file.read(fSize));

            file.close();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                 return false;
            }

            //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
            // This code was added for FUSE

            QString content = req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();

            utimbuf tb;
            tb.actime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;

            utime(filePath.toStdString().c_str(),&tb);
            //------------------------------------------------------------------------------------------------------------------


    }
    else{ // multipass uploading

        int cursorPosition=0;
        int blsz=0;
        qint64 fSize = file.size();

//        for(int i=0;i<passCount;i++){
        do{

            if(cursorPosition == 0){

                blsz=DROPBOX_CHUNK_SIZE;
            }
            else{

                if(fSize - cursorPosition < DROPBOX_CHUNK_SIZE){

                    blsz=fSize - cursorPosition;
                }
                else{

                    blsz=DROPBOX_CHUNK_SIZE;
                }

            }


            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/append");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert("session_id",sessID);
            cursor1->insert("offset",cursorPosition);


            req->addHeader("Dropbox-API-Arg",QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            //QByteArray* ba1=new QByteArray();

            file.seek(cursorPosition);
            //ba1->append(file.read(DROPBOX_CHUNK_SIZE));


            req->addHeader("Content-Length",QString::number(blsz).toLocal8Bit());
            req->post(file.read(blsz));

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

            delete(req);

            delete(cursor1);
            cursorPosition += blsz;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize - DROPBOX_CHUNK_SIZE){

                req=new MSRequest(this->proxyServer);
                //cursorPosition -= blsz;
                break;
            }

            req=new MSRequest(this->proxyServer);

        }while (true);

        // finalize upload

        req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Type",                      QString("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert("session_id",sessID);
        cursor.insert("offset",cursorPosition);
        QJsonObject commit;
        commit.insert("path",object->path+object->fileName);
        commit.insert("mode",QString("overwrite"));
        commit.insert("autorename",false);
        commit.insert("mute",false);
        commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert("cursor",cursor);
        doc.insert("commit",commit);

        req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
        //QByteArray ba;

        file.seek(cursorPosition);
        //ba.append(file.read(fSize-cursorPosition));


        req->addHeader("Content-Length",QString::number(fSize-cursorPosition).toLocal8Bit());
        req->post(file.read(fSize-cursorPosition));

        file.close();

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
            return false;
        }

        //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
        // This code was added for FUSE

        QString content = req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();

        utimbuf tb;
        tb.actime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;
        tb.modtime=(this->toMilliseconds(job["client_modified"].toString(),true))/1000;;

        utime(filePath.toStdString().c_str(),&tb);
        //------------------------------------------------------------------------------------------------------------------


    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob["id"].toString();

    if(rjob["id"].toString()==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
        return false;
    }

    delete(req);
    return true;
}

//=======================================================================================

bool MSDropbox::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return true;
    }

    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/2/files/create_folder");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;
    metaJson.insert("path",object->path+object->fileName);


    metaData.append((QJsonDocument(metaJson).toJson()));

    req->post(metaData);


    if(!req->replyOK()){
        if(!req->replyErrorText.contains("Conflict")){ // skip error on re-create existing folder

            req->printReplyError();
            delete(req);
            //exit(1);
            return false;
        }
        else{
            return true;
        }
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
    object->local.newRemoteID=job["id"].toString();
    object->remote.data=job;
    object->remote.exist=true;


    delete(req);

    if(job["id"].toString()==""){
        qStdOut()<< "Error when folder create "+filePath+" on remote" <<endl;
        return false;
    }


    this->filelist_populateChanges(*object);

    return true;
}

//=======================================================================================

void MSDropbox::remote_file_makeFolder(MSFSObject *object, const QString &parentID){
// obsolete
    // fixed warning message
    Q_UNUSED(object);
    Q_UNUSED(parentID);
}

//=======================================================================================

bool MSDropbox::remote_file_trash(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return true;
    }


    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/2/files/delete");


    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    QByteArray metaData;

    QJsonObject metaJson;
    metaJson.insert("path",object->path+object->fileName);


    metaData.append((QJsonDocument(metaJson).toJson()));

    req->post(metaData);

    if(!this->testReplyBodyForError(req->readReplyText())){
        QString errt=this->getReplyErrorString(req->readReplyText());

        if(! errt.contains("path_lookup/not_found/")){// ignore previous deleted files

            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
            delete(req);
            return false;
        }
        else{
            delete(req);
            return true;
        }


    }




    QString content=req->readReplyText();

    QString filePath=this->workPath+object->path+object->fileName;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    if(job["id"].toString()==""){
        qStdOut()<< "Error when move to trash "+filePath+" on remote" <<endl;
    }

    delete(req);

    return true;
}

//=======================================================================================

bool MSDropbox::remote_createDirectory(const QString &path){

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






// ============= LOCAL FUNCTIONS BLOCK =============
//=======================================================================================

void MSDropbox::local_createDirectory(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================

void MSDropbox::local_removeFile(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir trash(this->workPath+"/"+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+"/"+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+"/"+this->trashFileName+path;

    // create trashed folder structure if it's needed
    QFileInfo tfi(trashedPath);
    QDir tfs(tfi.absolutePath().replace(this->workPath,""));
    if(!tfs.exists()){
        tfs.mkdir(this->workPath + tfi.absolutePath().replace(this->workPath,""));
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

void MSDropbox::local_removeFolder(const QString &path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir trash(this->workPath+"/"+this->trashFileName);

    if(!trash.exists()){
        trash.mkdir(this->workPath+"/"+this->trashFileName);
    }

    QString origPath=this->workPath+path;
    QString trashedPath=this->workPath+"/"+this->trashFileName+path;

    // create trashed folder structure if it's needed
    QFileInfo tfi(trashedPath);
    QDir tfs(tfi.absolutePath().replace(this->workPath,""));
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



bool MSDropbox::directUpload(const QString &url, const QString &remotePath){

    // download file into temp file ---------------------------------------------------------------

    MSRequest *req = new MSRequest(this->proxyServer);

    QString tempFileName=this->generateRandom(10);
    QString filePath=this->workPath+"/"+tempFileName;

    req->setMethod("get");
    req->download(url,filePath);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    QFileInfo fileRemote(remotePath);
    QString path=fileRemote.absoluteFilePath();

    path=QString(path).left(path.lastIndexOf("/"));

    MSFSObject object;
    object.path=path+"/";
    object.fileName=fileRemote.fileName();
    object.getLocalMimeType(filePath);
    object.local.modifiedDate=   this->toMilliseconds(QFileInfo(filePath).lastModified(),true);

    //this->syncFileList.insert(object.path+object.fileName,object);

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



    delete(req);




    // upload file to remote ------------------------------------------------------------------------



    // start session ===========

    req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/start");
   // req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/octet-stream"));


    QByteArray mediaData;


    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    int passCount=fSize/DROPBOX_CHUNK_SIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return false;
    }

    req->post(mediaData);


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job["session_id"].toString();

    if(sessID==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
    }

    delete(req);

    req=new MSRequest(this->proxyServer);

    if(passCount==0){// onepass and finalize uploading

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert("session_id",sessID);
            cursor.insert("offset",0);
            QJsonObject commit;
            commit.insert("path",remotePath);
            commit.insert("mode",QString("overwrite"));
            commit.insert("autorename",false);
            commit.insert("mute",false);
            commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object.local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert("cursor",cursor);
            doc.insert("commit",commit);

            req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            QByteArray ba;

            ba.append(file.read(fSize));
            file.close();

            req->addHeader("Content-Length",QString::number(ba.length()).toLocal8Bit());
            req->post(ba);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                QFile rfile(filePath);
                rfile.remove();
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

    }
    else{ // multipass uploading

        int cursorPosition=0;

        for(int i=0;i<passCount;i++){

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/append");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert("session_id",sessID);
            cursor1->insert("offset",cursorPosition);


            req->addHeader("Dropbox-API-Arg",QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            QByteArray* ba1=new QByteArray();

            file.seek(cursorPosition);
            ba1->append(file.read(DROPBOX_CHUNK_SIZE));


            req->addHeader("Content-Length",QString::number(ba1->length()).toLocal8Bit());
            req->post(*ba1);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                QFile rfile(filePath);
                rfile.remove();
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

            delete(req);
            delete(ba1);
            delete(cursor1);
            cursorPosition += DROPBOX_CHUNK_SIZE;

            req=new MSRequest(this->proxyServer);
        }

        // finalize upload

        req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Type",                      QString("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert("session_id",sessID);
        cursor.insert("offset",cursorPosition);
        QJsonObject commit;
        commit.insert("path",remotePath);
        commit.insert("mode",QString("overwrite"));
        commit.insert("autorename",false);
        commit.insert("mute",false);
        commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object.local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert("cursor",cursor);
        doc.insert("commit",commit);

        req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
        QByteArray ba;

        file.seek(cursorPosition);
        ba.append(file.read(fSize-cursorPosition));
        file.close();

        req->addHeader("Content-Length",QString::number(ba.length()).toLocal8Bit());
        req->post(ba);

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            QFile rfile(filePath);
            rfile.remove();
            return false;
        }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                delete(req);
                return false;
            }

    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();


    if(rjob["id"].toString()==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
    }

    QFile rfile(filePath);
    rfile.remove();

    delete(req);

    return true;

}

QString MSDropbox::getInfo(){


    MSRequest *req0 = new MSRequest(this->proxyServer);

    req0->setRequestUrl("https://api.dropboxapi.com/2/users/get_current_account");
    req0->setMethod("post");

    req0->addHeader("Authorization",                     "Bearer "+this->access_token);

    req0->notUseContentType=true;

    req0->exec();


    if(!req0->replyOK()){
        req0->printReplyError();

        qStdOut()<< req0->replyText;
        delete(req0);
        return "false";
    }

    if(!this->testReplyBodyForError(req0->readReplyText())){
        qStdOut()<< "Service error. " << this->getReplyErrorString(req0->readReplyText()) << endl;
        delete(req0);
        return "false";
    }


    QString content0=req0->readReplyText();


    QJsonDocument json0 = QJsonDocument::fromJson(content0.toUtf8());
    QJsonObject job0 = json0.object();


    delete(req0);



    MSRequest *req = new MSRequest(this->proxyServer);

    req->setRequestUrl("https://api.dropboxapi.com/2/users/get_space_usage");
    req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);

    req->notUseContentType=true;

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();

        qStdOut()<< req->replyText;
        delete(req);
        return "false";
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
        delete(req);
        return "false";
    }


    QString content=req->readReplyText();


    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    if(job["allocation"].toObject().size()== 0){
        //qStdOut()<< "Error getting cloud information " <<endl;
        return "false";
    }

    delete(req);

    QJsonObject out;
    out["account"]=job0["email"].toString();
    out["total"]= QString::number( (uint64_t)job["allocation"].toObject()["allocated"].toDouble());
    out["usage"]= QString::number( job["used"].toInt());

    return QString( QJsonDocument(out).toJson());

}

