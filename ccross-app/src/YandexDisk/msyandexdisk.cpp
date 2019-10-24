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



#include "msyandexdisk.h"

#include <utime.h> // for macOS

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


#define YADISK_MAX_FILESIZE  8000000
//  150000000

MSYandexDisk::MSYandexDisk()
{
    this->providerName=     QStringLiteral("YandexDisk");
    this->tokenFileName=    QStringLiteral(".yadisk");
    this->stateFileName=    QStringLiteral(".yadisk_state");
    this->trashFileName=    QStringLiteral(".trash_yadisk");
}


//=======================================================================================

bool MSYandexDisk::auth(){


    connect(this,SIGNAL(oAuthCodeRecived(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));
    connect(this,SIGNAL(oAuthError(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));


    this->startListener(1973);

    qStdOut()<<"-------------------------------------"<<endl ;
    qStdOut()<< tr("Please go to this URL and confirm application credentials\n")<<endl ;


    qStdOut() << QStringLiteral("https://oauth.yandex.ru/authorize?force_confirm=yes&response_type=code&client_id=ba0517299fbc4e6db5ec7c040baccca7&state=") +this->generateRandom(20)<<endl ;



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

bool MSYandexDisk::onAuthFinished(const QString &html, MSCloudProvider *provider){

Q_UNUSED(provider)

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://oauth.yandex.ru/token"));
    req->setMethod(QStringLiteral("post"));

    req->addQueryItem(QStringLiteral("client_id"),          QStringLiteral("ba0517299fbc4e6db5ec7c040baccca7"));
    req->addQueryItem(QStringLiteral("client_secret"),      QStringLiteral("aed48ba99efb45a78f5138f63059bfc5"));
    req->addQueryItem(QStringLiteral("code"),               html.trimmed());
    req->addQueryItem(QStringLiteral("grant_type"),         QStringLiteral("authorization_code"));
    //req->addQueryItem("redirect_uri",       "http://127.0.0.1:1973");

    req->exec();


    if(!req->replyOK()){

        req->printReplyError();
        delete(req);
        this->providerAuthStatus=false;
        emit providerAuthComplete();
        return false;
    }

    QString content= req->replyText;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job[QStringLiteral("access_token")].toString();

    if(v!=""){

        this->token=v;
        qStdOut() << QStringLiteral("Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize.") <<endl;
        this->providerAuthStatus=true;
        emit providerAuthComplete();
        return true;
    }

    this->providerAuthStatus=false;
    emit providerAuthComplete();
    return false;
}

//=======================================================================================

void MSYandexDisk::saveTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << QStringLiteral("{\"access_token\" : \"")+this->token+QStringLiteral("\"}");
    key.close();

}


//=======================================================================================

bool MSYandexDisk::loadTokenFile(const QString &path){

    QFile key(path+QStringLiteral("/")+this->tokenFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << QStringLiteral("Access key missing or corrupt. Start CloudCross with -a option for obtained private key.")  <<endl ;
        return false;
    }

    QTextStream instream(&key);
    QString line;
    while(!instream.atEnd()){

        line+=instream.readLine();
    }

    QJsonDocument json = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject job = json.object();
    QString v=job[QStringLiteral("access_token")].toString();

    this->token=v;

    key.close();
    return true;
}


//=======================================================================================

void MSYandexDisk::loadStateFile(){

    QFile key(this->credentialsPath+"/"+this->stateFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << QStringLiteral("Previous state file not found. Start in stateless mode.")<<endl  ;
        return ;
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

void MSYandexDisk::saveStateFile(){


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



//=======================================================================================

bool MSYandexDisk::refreshToken(){

    this->access_token=this->token;
    return true;
}

//=======================================================================================

bool MSYandexDisk::createHashFromRemote(){

  //<<<<<<<<  this->readRemote();

    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/files/list_folder"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));




    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }




    QString list=req->readReplyText();




    delete(req);
    return true;
}


//=======================================================================================

bool MSYandexDisk::readRemote(const QString &currentPath){ //QString parentId,QString currentPath


    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net:443/v1/disk/resources"));
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));


    if(this->getFlag("lowMemory")){
        req->addQueryItem(QStringLiteral("limit"),      QStringLiteral("500"));
    }
    else{
        req->addQueryItem(QStringLiteral("limit"),      QStringLiteral("1000000"));
    }


    req->addQueryItem(QStringLiteral("offset"),          QStringLiteral("0"));
    req->addQueryItem(QStringLiteral("path"),          currentPath);


    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString list=req->readReplyText();

    delete(req);

    QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = json.object();
    QJsonArray entries= job[QStringLiteral("_embedded")].toObject()[QStringLiteral("items")].toArray();

    QFileInfo fi=QFileInfo(entries[0].toObject()[QStringLiteral("path")].toString()) ;

    int hasMore=entries.size();


    do{


        for(int i=0;i<entries.size();i++){

                    MSFSObject fsObject;

                    QJsonObject o=entries[i].toObject();

                    QString yPath=o[QStringLiteral("path")].toString();
                    yPath.replace(QStringLiteral("disk:/"),QStringLiteral("/"));

                    fsObject.path = QFileInfo(yPath).path()+QStringLiteral("/");

                    if(fsObject.path == QStringLiteral("//")){
                       fsObject.path =QStringLiteral("/");
                    }


                    // test for non first-level files


                    fsObject.remote.fileSize=  o[QStringLiteral("size")].toInt();
                    fsObject.remote.data=o;
                    fsObject.remote.exist=true;
                    fsObject.isDocFormat=false;

                    fsObject.state=MSFSObject::ObjectState::NewRemote;

                    if(this->isFile(o)){

                        fsObject.fileName=o[QStringLiteral("name")].toString();
                        fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                        fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("modified")].toString(),true);
                        fsObject.remote.md5Hash=o[QStringLiteral("md5")].toString();

                     }

                     if(this->isFolder(o)){

                         fsObject.fileName=o[QStringLiteral("name")].toString();
                         fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                         fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("modified")].toString(),true);

                         this->readRemote(yPath);
                      }

                      if(! this->filterServiceFileNames(yPath)){// skip service files and dirs

                          continue;
                      }


                      if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

                          if( this->filterIncludeFileNames(yPath)){

                              continue;
                          }
                      }
                      else{// use exclude by default

                          if(this->excludeList != QStringLiteral("")){
                              if(! this->filterExcludeFileNames(yPath)){

                                  continue;
                              }
                          }
                      }



                      this->syncFileList.insert(yPath, fsObject);

        }


            MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

            req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net:443/v1/disk/resources"));
            req->setMethod(QStringLiteral("get"));

            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));


            if(this->getFlag("lowMemory")){
                req->addQueryItem(QStringLiteral("limit"),      QStringLiteral("500"));
            }
            else{
                req->addQueryItem(QStringLiteral("limit"),      QStringLiteral("1000000"));
            }

            req->addQueryItem(QStringLiteral("offset"),          QString::number(entries.size()));
            req->addQueryItem(QStringLiteral("path"),          currentPath);

            req->exec();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }



            QString list=req->readReplyText();

            delete(req);

            json = QJsonDocument::fromJson(list.toUtf8());
            job = json.object();
            entries= job[QStringLiteral("_embedded")].toObject()[QStringLiteral("items")].toArray();

            hasMore=entries.size();




    }while(hasMore > 0);

    return true;

}

bool MSYandexDisk::_readRemote(const QString &rootPath){

    Q_UNUSED(rootPath)
//    return this->readRemote(rootPath);
    return this->readRemote("/");
}



//=======================================================================================


bool MSYandexDisk::isFile(const QJsonValue &remoteObject){
    return remoteObject.toObject()[QStringLiteral("type")].toString()==QStringLiteral("file");
}

//=======================================================================================

bool MSYandexDisk::isFolder(const QJsonValue &remoteObject){
    return remoteObject.toObject()[QStringLiteral("type")].toString()==QStringLiteral("dir");
}

//=======================================================================================

bool MSYandexDisk::createSyncFileList(){

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
                if(instream.pos() == 7 && line == QStringLiteral("regexp")){
                    this->options.insert(QStringLiteral("filter-type"), QStringLiteral("regexp"));
                    continue;
                }
                this->includeList=this->includeList+line+QStringLiteral("|");
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
                if(instream.pos() == 7 && line == QStringLiteral("regexp")){
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

    if(this->getFlag("noSync")){
        qStdOut() << "Synchronization capability was disabled."<<endl;
    }
    else{
        qStdOut()<< QStringLiteral("Reading remote files")<<endl ;


        if(!this->readRemote(QStringLiteral("/"))){// top level files and folders
            qStdOut()<<QStringLiteral("Error occured on reading remote files")<<endl  ;
            return false;

        }
    }


    qStdOut()<<QStringLiteral("Reading local files and folders")<<endl  ;

    if(!this->readLocal(this->workPath)){
        qStdOut()<<QStringLiteral("Error occured on local files and folders") <<endl ;
        return false;

    }

//this->remote_file_get(&(this->syncFileList.values()[0]));
//this->remote_file_insert(&(this->syncFileList.values()[0]));
//this->remote_file_update(&(this->syncFileList.values()[0]));
// this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
//    this->remote_file_trash(&(this->syncFileList.values()[0]));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));

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


//        QHash<QString,MSFSObject> L1;
//        QHash<QString,MSFSObject> L2;
//        QHash<QString,MSFSObject> L3;

        MSSyncThread* threads[3] = {thr1, thr2, thr3};
        int j = 0;
        for(const auto& key : keys){
            threads[j++]->threadSyncList.insert(key,this->syncFileList.find(key).value());
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

//=======================================================================================

bool MSYandexDisk::readLocal(const QString &path){



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



            if(i!=this->syncFileList.end()){// if object exists in Yandex Disk

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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
                fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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

bool MSYandexDisk::readLocalSingle(const QString &path){

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



            if(i!=this->syncFileList.end()){// if object exists in Yandex Disk

                MSFSObject* fsObject = &(i.value());


                fsObject->local.fileSize=  fi.size();
                fsObject->local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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
                fsObject.local.md5Hash= this->fileChecksum(Path,QCryptographicHash::Md5);
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


MSFSObject::ObjectState MSYandexDisk::filelist_defineObjectState(const MSLocalFSObject &local, const MSRemoteFSObject &remote){


    if((local.exist)&&(remote.exist)){ //exists both files

        if(local.md5Hash==remote.md5Hash){


                return MSFSObject::ObjectState::Sync;

        }


        // compare last modified date for local and remote
        if(local.modifiedDate==remote.modifiedDate){

           // return MSFSObject::ObjectState::Sync;

            if(this->strategy==MSCloudProvider::SyncStrategy::PreferLocal){
                return MSFSObject::ObjectState::ChangedLocal;
            }

            return MSFSObject::ObjectState::ChangedRemote;

        }

        if(local.modifiedDate > remote.modifiedDate){
            return MSFSObject::ObjectState::ChangedLocal;
        }

        return MSFSObject::ObjectState::ChangedRemote;

    }


    if((local.exist)&&(!remote.exist)){ //exist only local file

        if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){
            return  MSFSObject::ObjectState::NewLocal;
        }

        return  MSFSObject::ObjectState::DeleteRemote;
    }


    if((!local.exist)&&(remote.exist)){ //exist only remote file

        if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){
            return  MSFSObject::ObjectState::DeleteLocal;
        }

        return  MSFSObject::ObjectState::NewRemote;
    }

    return  MSFSObject::ObjectState::ErrorState;
}

//=======================================================================================


void MSYandexDisk::checkFolderStructures(){

    QHash<QString,MSFSObject>::iterator lf;

    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

        // create new folder structure on remote

        qStdOut()<<QStringLiteral("Checking folder structure on remote")<<endl  ;

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

        qStdOut()<<QStringLiteral("Checking folder structure on local") <<endl ;

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


void MSYandexDisk::doSync(QHash<QString, MSFSObject> fsObjectList){

    QHash<QString,MSFSObject>::iterator lf;


    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag("force")){

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

                qStdOut()<<QStringLiteral("Start uploading in force mode")<<endl ;

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

                                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced uploading.") ) <<endl;

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



            qStdOut()<<QStringLiteral("Syncronization end") <<endl ;

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

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote."))<<endl  ;

                        this->remote_file_trash(&obj);

                    }
                    else{
                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") )<<endl ;

                        this->remote_file_get(&obj);
                    }


                }
                else{

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") )<<endl ;

                        this->remote_file_trash(&obj);
                    }
                    else{

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") ) <<endl;

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

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Delete local. Deleting remote.") )<<endl ;

                this->remote_file_trash(&obj);

                break;

            case MSFSObject::ObjectState::DeleteRemote:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    if(this->strategy == MSCloudProvider::SyncStrategy::PreferLocal){

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") ) <<endl;

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


//bool MSYandexDisk::remote_file_generateIDs(int count) {
//// absolete
//    // fix warning message
//    count++;
//    return false;
//}

QHash<QString,MSFSObject>   MSYandexDisk::filelist_getFSObjectsByState(MSFSObject::ObjectState state) {

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


QHash<QString,MSFSObject>   MSYandexDisk::filelist_getFSObjectsByState(QHash<QString, MSFSObject> fsObjectList, MSFSObject::ObjectState state) {

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


QHash<QString,MSFSObject>   MSYandexDisk::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type){


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


QHash<QString,MSFSObject>   MSYandexDisk::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type){

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

bool MSYandexDisk::filelist_FSObjectHasParent(const MSFSObject &fsObject){
//    if(fsObject.path==QStringLiteral("/")){
//        return false;
//    }
//    else{
//        return true;
//    }

    return (fsObject.path.count(QStringLiteral("/"))>=1)&&(fsObject.path!=QStringLiteral("/"));

}

//=======================================================================================



MSFSObject MSYandexDisk::filelist_getParentFSObject(const MSFSObject &fsObject){

    QString parentPath;

    if((fsObject.local.objectType==MSLocalFSObject::Type::file) || (fsObject.remote.objectType==MSRemoteFSObject::Type::file)){
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf(QStringLiteral("/")));
    }
    else{
        parentPath=fsObject.path.left(fsObject.path.lastIndexOf(QStringLiteral("/")));
    }

    if(parentPath==QStringLiteral("")){
        parentPath=QStringLiteral("/");
    }

    QHash<QString,MSFSObject>::iterator parent=this->syncFileList.find(parentPath);

    if(parent != this->syncFileList.end()){
        return parent.value();
    }

    return MSFSObject();
}


void MSYandexDisk::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}



bool MSYandexDisk::testReplyBodyForError(const QString &body) {

        QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
        QJsonObject job = json.object();

        QJsonValue e=(job[QStringLiteral("error")]);
        return e.isNull();


}


QString MSYandexDisk::getReplyErrorString(const QString &body) {

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job[QStringLiteral("description")]);

    return e.toString();

}



//=======================================================================================

// download file from cloud
bool MSYandexDisk::remote_file_get(MSFSObject* object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.")<<endl  ;
        return true;
    }


    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources/download"));
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));
    req->addQueryItem(QStringLiteral("path"),                           object->path + object->fileName);

    QString filePath=this->workPath+object->path+ CCROSS_TMP_PREFIX +object->fileName;

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();


    if(this->testReplyBodyForError(req->readReplyText())){

        if(object->remote.objectType==MSRemoteFSObject::Type::file){

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();
            QString href=job[QStringLiteral("href")].toString();

            this->local_writeFileContent(filePath,href);// call overloaded from this

            // set remote "change time" for local file

//            utimbuf tb;
//            tb.actime=(this->toMilliseconds(object->remote.data["client_modified"].toString(),true))/1000;;
//            tb.modtime=(this->toMilliseconds(object->remote.data["client_modified"].toString(),true))/1000;;

//            utime(filePath.toStdString().c_str(),&tb);
        }
    }
    else{

        if(! this->getReplyErrorString(req->readReplyText()).contains( "path/not_file/")){
            qStdOut() << QString(QStringLiteral("Service error. ")+ this->getReplyErrorString(req->readReplyText()))<<endl;
            delete(req);
            return false;
        }
    }


    delete(req);
    return true;

}

//=======================================================================================

bool MSYandexDisk::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< QString(object->fileName + QStringLiteral(" is a folder. Skipped.") )<<endl ;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    // get url for uploading ===========

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources/upload"));
    req->setMethod("get");

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));
#ifndef CCROSS_LIB
    req->addQueryItem(QStringLiteral("path"),                           QUrl::toPercentEncoding(object->path+object->fileName));
#else
    req->addQueryItem("path",                           QString(object->path + object->fileName)/*.replace("/","%2f")*/);
#endif

    req->addQueryItem(QStringLiteral("overwrite"),                           QStringLiteral("true"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString href=job[QStringLiteral("href")].toString();



    delete(req);

    // upload file content

    QString filePath=  this->workPath+object->path+object->fileName;

    QFile file(filePath);
    qint64 fSize=file.size();

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(href);
    req->setMethod("put");


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QString(QStringLiteral("Unable to open of ")+filePath)<<endl  ;
        delete(req);
        return false;
    }
    file.close();

    req->addHeader(QStringLiteral("Content-Type"),"");
    req->addHeader(QStringLiteral("Content-Length"),QString::number(fSize).toLocal8Bit());

    req->setInputDataStream(&file);
    req->exec();
//    req->put(&file);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }




    delete(req);
    return true;
}


//=======================================================================================

bool MSYandexDisk::remote_file_update(MSFSObject *object, const char *newParameter){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< QString(object->fileName + QStringLiteral(" is a folder. Skipped.") )<<endl ;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    // get url for uploading ===========

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(newParameter);
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));
    req->addQueryItem(QStringLiteral("path"),                           object->path+object->fileName);
    req->addQueryItem(QStringLiteral("overwrite"),                           QStringLiteral("true"));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString href=job[QStringLiteral("href")].toString();



    delete(req);

    // upload file content

    QString filePath=  this->workPath+object->path+object->fileName;

    QFile file(filePath);
    qint64 fSize=file.size();

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(href);
    req->setMethod("put");


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath <<endl ;
        delete(req);
        return false;
    }


    req->addHeader(QStringLiteral("Content-Length"),QString::number(fSize).toLocal8Bit());

    req->setInputDataStream(&file);
    req->exec();
//    req->put(&file);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }




    delete(req);
    return true;
}

//=======================================================================================

bool MSYandexDisk::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setMethod("put");

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addQueryItem(QStringLiteral("path"),                           QUrl::toPercentEncoding(object->path+object->fileName));

    QByteArray metaData;


    req->setInputDataStream(metaData);
    req->exec();
//    req->put(metaData);


    if(!req->replyOK()){

        if(!req->replyErrorText.contains(QStringLiteral("CONFLICT"))){ // skip error on re-create existing folder

            req->printReplyError();
            delete(req);
            //exit(1);
            return false;
        }

        delete(req);
        return true;

    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QString(QStringLiteral("Service error. ") + this->getReplyErrorString(req->readReplyText()))<<endl;
        delete(req);
        return false;
    }



    // getting meta info of created folder


    QString filePath=this->workPath+object->path+object->fileName;

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();


    QString href=job[QStringLiteral("href")].toString();

    delete(req);

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(href);
    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
//    req->addQueryItem(QStringLiteral("path"),                           QUrl::toPercentEncoding(object->path+object->fileName));
    req->setMethod(QStringLiteral("get"));

    req->exec();

    content=req->readReplyText();

    json = QJsonDocument::fromJson(content.toUtf8());
    job = json.object();

   // object->local.newRemoteID=job["id"].toString();
    object->remote.data=job;
    object->remote.exist=true;

    if(job[QStringLiteral("path")].toString()==QStringLiteral("")){
        qStdOut()<< QString(QStringLiteral("Error when folder create ")+filePath+QStringLiteral(" on remote")) <<endl ;
    }

    delete(req);

    this->filelist_populateChanges(*object);

    return true;

}

//=======================================================================================

void MSYandexDisk::remote_file_makeFolder(MSFSObject *object, const QString &parentID){
// obsolete
    //fix warning message
    Q_UNUSED(object);
    Q_UNUSED(parentID);

}

//=======================================================================================

bool MSYandexDisk::remote_file_trash(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources"));
    req->setMethod("delete");


    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    //req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));
    req->addQueryItem(QStringLiteral("path"),                           QUrl::toPercentEncoding(object->path+object->fileName));

    req->exec();
//    req->deleteResource();

    if(!this->testReplyBodyForError(req->readReplyText())){
        QString errt=this->getReplyErrorString(req->readReplyText());

        if(! errt.contains(QStringLiteral("Resource not found"))){// ignore previous deleted files

            qStdOut()<< QString(QStringLiteral("Service error. ") + this->getReplyErrorString(req->readReplyText()) )<<endl;
            delete(req);
            return false;
        }

        delete(req);
        return true;
    }




    delete(req);
    return true;
}

//=======================================================================================

bool MSYandexDisk::remote_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    QList<QString> dirs=path.split(QStringLiteral("/"));
    QString currPath=QStringLiteral("");

    for(int i=1;i<dirs.size();i++){

        QHash<QString,MSFSObject>::iterator f=this->syncFileList.find(currPath+"/"+dirs[i]);

        if(f != this->syncFileList.end()){

            currPath=f.key();

            if(f.value().remote.exist){
                continue;
            }

            if(this->filelist_FSObjectHasParent(f.value())){

//                MSFSObject parObj=this->filelist_getParentFSObject(f.value());
//                this->remote_file_makeFolder(&f.value(),parObj.remote.data["id"].toString());
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

void MSYandexDisk::local_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================

void MSYandexDisk::local_removeFile(const QString &path){

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
        this->createDirectoryPath(this->workPath + tfi.absolutePath().replace(this->workPath,""));
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

void MSYandexDisk::local_removeFolder(const QString &path){

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
        this->createDirectoryPath(this->workPath + tfi.absolutePath().replace(this->workPath,""));
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

//=======================================================================================

bool MSYandexDisk::local_writeFileContent(const QString &filePath, const QString &hrefToDownload){


    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(hrefToDownload);

    req->setMethod(QStringLiteral("get"));

    req->setOutputFile(filePath);
    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);

        return false;
    }

    this->local_actualizeTempFile(filePath);


    return true;
}



bool MSYandexDisk::directUpload(const QString &url, const QString &remotePath){

    qStdOut() << "NOT INPLEMENTED YET"<<endl;
    return false;


    // download file into temp file ---------------------------------------------------------------

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    QString filePath=this->workPath+"/"+this->generateRandom(10);

    req->setMethod("get");
    req->download(url,filePath);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        //exit(1);
        return false;
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



    delete(req);

    // upload file to remote ------------------------------------------------------------------------


    // get url for uploading ===========

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl("https://cloud-api.yandex.net/v1/disk/resources/upload");
    req->setMethod("get");

    req->addHeader("Authorization",                     "OAuth "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));
    req->addQueryItem("path",                           object.path+object.fileName);
    req->addQueryItem("overwrite",                           "true");

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
       // exit(1);
        return false;
    }



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString href=job["href"].toString();



    delete(req);

    // upload file content

   // filePath=  this->workPath+object.path+object.fileName;

    QFile file(filePath);
    qint64 fSize=file.size();

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(href);


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QString("Unable to open of "+filePath)<<endl  ;
        delete(req);
        //exit(1);
        return false;
    }


    req->addHeader("Content-Length",QString::number(fSize).toLocal8Bit());
    req->put(&file);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        //exit(1);
        return false;
    }


    file.remove();

    delete(req);

    return true;
}




QString MSYandexDisk::getInfo(){

    MSHttpRequest *req0 = new MSHttpRequest(this->proxyServer);

    req0->setRequestUrl(QStringLiteral("https://login.yandex.ru/info"));
    req0->setMethod(QStringLiteral("get"));

   // req0->addHeader("Authorization",                     "OAuth "+this->access_token);
   // req0->addHeader("Accept:",                      QString("*/*"));

    req0->addQueryItem(QStringLiteral("oauth_token"),QString(this->access_token));


    req0->exec();


    if(!req0->replyOK()){
        req0->printReplyError();

        qStdOut()<< req0->replyText<<endl;
        delete(req0);
        return QStringLiteral("false");
    }

    if(!this->testReplyBodyForError(req0->readReplyText())){
        qStdOut()<< QString(QStringLiteral("Service error. ") + this->getReplyErrorString(req0->readReplyText()) )<<endl;
        delete(req0);
        return QStringLiteral("false");
    }


    QString content0=req0->readReplyText();

    QJsonDocument json0 = QJsonDocument::fromJson(content0.toUtf8());
    QJsonObject job0 = json0.object();





    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/"));
    req->setMethod(QStringLiteral("get"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));



    req->exec();


    if(!req->replyOK()){
        req->printReplyError();

        qStdOut()<< req->replyText<<endl;
        delete(req);
        return QStringLiteral("false");
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< QString(QStringLiteral("Service error. ") + this->getReplyErrorString(req->readReplyText()) )<<endl;
        delete(req);
        return QStringLiteral("false");
    }


    QString content=req->readReplyText();
    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();

    if(((uint64_t)job[QStringLiteral("total_space")].toDouble()) == 0){
        //qStdOut()<< "Error getting cloud information "  ;
        return QStringLiteral("false");
    }


    QJsonObject out;
    out[QStringLiteral("account")]=job0[QStringLiteral("login")].toString()+QStringLiteral("@yandex.ru");
    out[QStringLiteral("total")]= QString::number( (uint64_t)job[QStringLiteral("total_space")].toDouble());
    out[QStringLiteral("usage")]= QString::number( job[QStringLiteral("used_space")].toInt());

    return QString( QJsonDocument(out).toJson());

}


bool MSYandexDisk::remote_file_empty_trash(){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://cloud-api.yandex.net/v1/disk/trash/resources"));
    req->setMethod(QStringLiteral("delete"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("OAuth ")+this->access_token);

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
