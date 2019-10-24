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

#include <utime.h> // for macOS

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


//  150000000

MSDropbox::MSDropbox()
{
    this->providerName=     QStringLiteral("Dropbox");
    this->tokenFileName=    QStringLiteral(".dbox");
    this->stateFileName=    QStringLiteral(".dbox_state");
    this->trashFileName=    QStringLiteral(".trash_dbox");
}


//=======================================================================================

bool MSDropbox::auth(){

    connect(this,SIGNAL(oAuthCodeRecived(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));
    connect(this,SIGNAL(oAuthError(QString,MSCloudProvider*)),this,SLOT(onAuthFinished(QString, MSCloudProvider*)));


    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    //www.dropbox.com/1/oauth2/authorize?client_id=jqw8z760uh31l92&response_type=code&state=hGDSAGDKJASHDKJHASLKFH

    req->setRequestUrl(QStringLiteral("https://www.dropbox.com/1/oauth2/authorize"));
    req->setMethod("get");

    req->addQueryItem(QStringLiteral("redirect_uri"),           QStringLiteral("http://127.0.0.1:1973"));
    req->addQueryItem(QStringLiteral("response_type"),          QStringLiteral("code"));
    req->addQueryItem(QStringLiteral("client_id"),              QStringLiteral("jqw8z760uh31l92"));
    req->addQueryItem(QStringLiteral("state"),                  this->generateRandom(20));

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        this->providerAuthStatus=false;
        return false;
    }

    this->startListener(1973);

    qStdOut()<<"-------------------------------------" <<endl;
    qStdOut()<< tr("Please go to this URL and confirm application credentials\n") <<endl;

    qStdOut() << req->replyURL()<<endl;
    qStdOut()<<""<<endl;


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


bool MSDropbox::onAuthFinished(const QString &html, MSCloudProvider *provider){

    Q_UNUSED(provider)

    QString code = html.trimmed();


    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/1/oauth2/token"));
    req->setMethod(QStringLiteral("post"));

    req->addQueryItem(QStringLiteral("client_id"),          QStringLiteral("jqw8z760uh31l92"));
    req->addQueryItem(QStringLiteral("client_secret"),      QStringLiteral("eeuwcu3kzqrbl9j"));
    req->addQueryItem(QStringLiteral("code"),               code);
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

//    if(!this->testReplyBodyForError(req->readReplyText())){
//        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
//        exit(0);
//    }


    QString content= req->replyText;//lastReply->readAll();

    //qStdOut() << content;


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
    outk << QStringLiteral("{\"access_token\" : \"")+this->token+QStringLiteral("\"}");
    key.close();

}


//=======================================================================================

bool MSDropbox::loadTokenFile(const QString &path){

    QFile key(path+"/"+this->tokenFileName);

    if(!key.open(QIODevice::ReadOnly))
    {
        qStdOut() << QStringLiteral("Access key missing or corrupt. Start CloudCross with -a option for obtained private key.") <<endl  ;
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

void MSDropbox::loadStateFile(){

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
}


//=======================================================================================

void MSDropbox::saveStateFile(){


    QJsonDocument state;
    QJsonObject jso;
    jso.insert(QStringLiteral("change_stamp"),QStringLiteral("0"));

    QJsonObject jts;
    jts.insert(QStringLiteral("nsec"),QStringLiteral("0"));
    jts.insert(QStringLiteral("sec"),QString::number(QDateTime( QDateTime::currentDateTime()).toMSecsSinceEpoch()));

    jso.insert(QStringLiteral("last_sync"),jts);
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

bool MSDropbox::readRemote(){ //QString parentId,QString currentPath


    MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/files/list_folder"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));


    QJsonObject d;

    d.insert(QStringLiteral("path"),QStringLiteral(""));
    d.insert(QStringLiteral("recursive"),true);
    d.insert(QStringLiteral("include_media_info"),false);
    d.insert(QStringLiteral("include_deleted"),false);

    if(this->getFlag("lowMemory")){
        d.insert(QStringLiteral("limit"),500);
    }


    QByteArray metaData;

    metaData.append((QJsonDocument(d).toJson()));

    req->addHeader(QStringLiteral("Content-Length"),QString::number(metaData.length()).toLocal8Bit());

//    req->post(metaData);
    req->setInputDataStream(metaData);
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
    QJsonArray entries= job[QStringLiteral("entries")].toArray();

    QFileInfo fi=QFileInfo(entries[0].toObject()[QStringLiteral("path_display")].toString()) ;

    bool hasMore=(job[QStringLiteral("has_more")].toBool());

    do{


        for(int i=0;i<entries.size();i++){

                    MSFSObject fsObject;

                    QJsonObject o=entries[i].toObject();

                    fsObject.path = QFileInfo(o[QStringLiteral("path_display")].toString()).path()+"/";

                    if(fsObject.path == QStringLiteral("//")){
                       fsObject.path =QStringLiteral("/");
                    }

                    fsObject.remote.fileSize=  o[QStringLiteral("size")].toInt();
                    fsObject.remote.data=o;
                    fsObject.remote.exist=true;
                    fsObject.isDocFormat=false;

                    fsObject.state=MSFSObject::ObjectState::NewRemote;

                    if(this->isFile(o)){

                        fsObject.fileName=o[QStringLiteral("name")].toString();
                        fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                        fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("client_modified")].toString(),true);
                     }

                     if(this->isFolder(o)){

                         fsObject.fileName=o[QStringLiteral("name")].toString();
                         fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                         fsObject.remote.modifiedDate=this->toMilliseconds(o[QStringLiteral("client_modified")].toString(),true);

                      }

                      if(! this->filterServiceFileNames(o[QStringLiteral("path_display")].toString())){// skip service files and dirs

                          continue;
                      }


                      if(this->getFlag(QStringLiteral("useInclude")) && this->includeList != QStringLiteral("")){//  --use-include

                          if(this->filterIncludeFileNames(o[QStringLiteral("path_display")].toString())){
                              i++;
                              continue;
                          }
                      }
                      else{// use exclude by default

                          if(this->excludeList != ""){
                              if(!this->filterExcludeFileNames(o[QStringLiteral("path_display")].toString())){
                                  i++;
                                  continue;
                              }
                          }
                      }


                      this->syncFileList.insert(o[QStringLiteral("path_display")].toString(), fsObject);

        }

        hasMore=(job[QStringLiteral("has_more")].toBool());

        if(hasMore){

            MSHttpRequest* req=new MSHttpRequest(this->proxyServer);

            req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/files/list_folder/continue"));
            req->setMethod(QStringLiteral("post"));

            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

            QJsonObject d;

            d.insert(QStringLiteral("cursor"),job[QStringLiteral("cursor")].toString());

            QByteArray metaData;

            metaData.append(QString(QJsonDocument(d).toJson()).toLocal8Bit());

            req->addHeader(QStringLiteral("Content-Length"),QString::number(metaData.length()).toLocal8Bit());

//            req->post(metaData);
            req->setInputDataStream(metaData);
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
            entries= job[QStringLiteral("entries")].toArray();

            hasMore=true;//(job["has_more"].toBool());

        }



    }while(hasMore);


    return true;
}

bool MSDropbox::_readRemote(const QString &rootPath){

    Q_UNUSED(rootPath);

    return this->readRemote();

}



//=======================================================================================


bool MSDropbox::isFile(const QJsonValue &remoteObject){
    return remoteObject.toObject()[".tag"].toString()=="file";
}

//=======================================================================================

bool MSDropbox::isFolder(const QJsonValue &remoteObject){
    return remoteObject.toObject()[QStringLiteral(".tag")].toString()==QStringLiteral("folder");
}

//=======================================================================================

bool MSDropbox::createSyncFileList(){

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

        if(!this->readRemote()){// top level files and folders
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
 //this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
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

//=======================================================================================


bool MSDropbox::readLocal(const QString &path){



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

                if(relPath.lastIndexOf(QStringLiteral("/"))==0){
                    fsObject.path=QStringLiteral("/");
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf(QStringLiteral("/")))+QStringLiteral("/");
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


void MSDropbox::doSync(QHash<QString, MSFSObject> fsObjectList){

    QHash<QString,MSFSObject>::iterator lf;


    // FORCING UPLOAD OR DOWNLOAD FILES AND FOLDERS
    if(this->getFlag(QStringLiteral("force"))){

        if(this->getOption(QStringLiteral("force"))==QStringLiteral("download")){

            qStdOut()<<QStringLiteral("Start downloading in force mode") <<endl ;

            lf=fsObjectList.begin();

            for(;lf != fsObjectList.end();lf++){

                MSFSObject obj=lf.value();

                if((obj.state == MSFSObject::ObjectState::Sync)||
                   (obj.state == MSFSObject::ObjectState::NewRemote)||
                   (obj.state == MSFSObject::ObjectState::DeleteLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedLocal)||
                   (obj.state == MSFSObject::ObjectState::ChangedRemote) ){


                    if(obj.remote.objectType == MSRemoteFSObject::Type::file){

                        qStdOut()<< QString(obj.path+obj.fileName +QString(" Forced downloading.") )<<endl ;

                        this->remote_file_get(&obj);
                    }
                }

            }

        }
        else{
            if(this->getOption(QStringLiteral("force"))==QStringLiteral("upload")){

                qStdOut()<<QStringLiteral("Start uploading in force mode") <<endl ;

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

                                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced uploading.") ) <<endl;

                                this->remote_file_update(&obj);
                            }
                        }
                        else{

                            if(obj.local.objectType == MSLocalFSObject::Type::file){

                                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Forced uploading.") )<<endl;

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

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Changed local. Uploading.") ) <<endl;

                this->remote_file_update(&obj);

                break;

            case MSFSObject::ObjectState::NewLocal:

                if((obj.local.modifiedDate > this->lastSyncTime)&&(this->lastSyncTime != 0)){// object was added after last sync

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") ) <<endl;

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

                qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" Changed remote. Downloading.") ) <<endl;

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

                    qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New remote. Downloading.") ) <<endl;

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

                        qStdOut()<< QString(obj.path+obj.fileName +QStringLiteral(" New local. Uploading.") ) <<endl;

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




        qStdOut()<<QStringLiteral("Syncronization end")  <<endl;

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
//    if(fsObject.path==QStringLiteral("/")){
//        return false;
//    }
//    else{
//        return true;
//    }

    return (fsObject.path.count(QStringLiteral("/"))>=1)&&(fsObject.path!=QStringLiteral("/"));

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


void MSDropbox::filelist_populateChanges(const MSFSObject &changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}



bool MSDropbox::testReplyBodyForError(const QString &body) {

    return !body.contains(QStringLiteral("\"error\": {"));

}


QString MSDropbox::getReplyErrorString(const QString &body) {

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job[QStringLiteral("error_summary")]);

    return e.toString();

}



//=======================================================================================

// download file from cloud
bool MSDropbox::remote_file_get(MSFSObject* object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    if(object->remote.objectType == MSRemoteFSObject::Type::folder){

         qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.") <<endl ;
         return true;
     }

    QString id=object->remote.data["id"].toString();

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/download"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Dropbox-API-Arg"),QStringLiteral("{\"path\":\"")+id+QStringLiteral("\"}"));
    req->addHeader(QStringLiteral("Content-Type"),QByteArray()); // this line needed for requests with libcurl version of MSHttpRequest

    QString filePath=this->workPath+object->path + CCROSS_TMP_PREFIX + object->fileName;



    req->setOutputFile(filePath);
    req->exec();


    if(this->testReplyBodyForError(req->readReplyText())){

        if(object->remote.objectType==MSRemoteFSObject::Type::file){

            this->local_actualizeTempFile(filePath);

            // set remote "change time" for local file

            utimbuf tb;
            tb.actime=(this->toMilliseconds(object->remote.data[QStringLiteral("client_modified")].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(object->remote.data[QStringLiteral("client_modified")].toString(),true))/1000;;

            filePath = this->workPath+object->path + object->fileName;
            utime(filePath.toLocal8Bit().constData(),&tb);
        }
    }
    else{

        if(! this->getReplyErrorString(req->readReplyText()).contains( QStringLiteral("path/not_file/"))){
            qStdOut() << QStringLiteral("Service error. ")<< this->getReplyErrorString(req->readReplyText())<<endl;
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

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.")  <<endl;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    // start session ===========

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/start"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    //qint64 fSize=file.size();
//    unsigned int passCount=(unsigned int)((unsigned int)fSize / (unsigned int)DROPBOX_CHUNK_SIZE);// count of 150MB blocks


    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath <<endl ;
        delete(req);
        return false;
    }

//    req->post(mediaData);
    req->setInputDataStream(mediaData);
    req->exec();



    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job[QStringLiteral("session_id")].toString();

    if(sessID==""){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")  <<endl;
        delete(req);
        return false;
    }

    delete(req);

    req=new MSHttpRequest(this->proxyServer);

    if( file.size() <= DROPBOX_CHUNK_SIZE){// onepass and finalize uploading

            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
            req->setMethod("post");
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert(QStringLiteral("session_id"),sessID);
            cursor.insert(QStringLiteral("offset"),0);
            QJsonObject commit;
            commit.insert(QStringLiteral("path"),object->path+object->fileName);
            commit.insert(QStringLiteral("mode"),QStringLiteral("add"));
            commit.insert(QStringLiteral("autorename"),false);
            commit.insert(QStringLiteral("mute"),false);
            commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert(QStringLiteral("cursor"),cursor);
            doc.insert(QStringLiteral("commit"),commit);

            req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
//            QByteArray ba;

//            ba.append(file.read(fSize));


            req->addHeader(QStringLiteral("Content-Length"),QString::number(file.size()).toLocal8Bit());

//            req->post(file.read(fSize));
            req->setInputDataStream(&file);
            req->exec();

//            req->post(&file);


            file.close();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                return false;
            }

            //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
            // This code was added for FUSE

            QString content = req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();

            utimbuf tb;
            tb.actime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;

            utime(filePath.toLocal8Bit().constData(),&tb);
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

            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/append"));
            req->setMethod("post");
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert(QStringLiteral("session_id"),sessID);
            cursor1->insert(QStringLiteral("offset"),cursorPosition);


            req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            //QByteArray* ba1=new QByteArray();

//            file.seek(cursorPosition);
            //ba1->append(file.read(DROPBOX_CHUNK_SIZE));



            req->addHeader(QStringLiteral("Content-Length"),QString::number(blsz).toLocal8Bit());
//            req->post(file.read(blsz));

            req->setPayloadChunkData(blsz,cursorPosition);
            req->setInputDataStream(&file);
            req->exec();

            //qStdOut()<< blsz;

//            req->post(&file);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

            delete(req);

            delete(cursor1);
            cursorPosition += blsz;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize - DROPBOX_CHUNK_SIZE){

                req=new MSHttpRequest(this->proxyServer);
                //cursorPosition -= blsz;
                break;
            }

            req=new MSHttpRequest(this->proxyServer);
        }while(true);

        // finalize upload

        req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
        req->setMethod("post");
        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert(QStringLiteral("session_id"),sessID);
        cursor.insert(QStringLiteral("offset"),cursorPosition);
        QJsonObject commit;
        commit.insert(QStringLiteral("path"),object->path+object->fileName);
        commit.insert(QStringLiteral("mode"),QStringLiteral("add"));
        commit.insert(QStringLiteral("autorename"),false);
        commit.insert(QStringLiteral("mute"),false);
        commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert(QStringLiteral("cursor"),cursor);
        doc.insert(QStringLiteral("commit"),commit);

        req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
        //QByteArray ba;

//        file.seek(cursorPosition);
        //ba.append(file.read(fSize-cursorPosition));


        req->addHeader(QStringLiteral("Content-Length"),QString::number(fSize-cursorPosition).toLocal8Bit());
//        req->post(file.read(fSize-cursorPosition));

        req->setPayloadChunkData(fSize-cursorPosition,cursorPosition);
//        req->post(&file);

        req->setInputDataStream(&file);
        req->exec();

//        qStdOut()<< "finalize "<<fSize-cursorPosition;
//        req->setPayloadChunkSize(fSize-cursorPosition);
//        req->post(&file);

        file.close();

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
            return false;
        }

        //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
        // This code was added for FUSE

        QString content = req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();

        utimbuf tb;
        tb.actime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;
        tb.modtime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;

        utime(filePath.toLocal8Bit().constData(),&tb);
        //------------------------------------------------------------------------------------------------------------------


    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob["id"].toString();

    if(rjob[QStringLiteral("id")].toString()==""){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote") <<endl ;
        return false;
    }

    delete(req);
    return true;
}


//=======================================================================================

bool MSDropbox::remote_file_update(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << QStringLiteral(" is a folder. Skipped.")<<endl  ;
        return true;
    }

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    // start session ===========

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/start"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    //qint64 fSize=file.size();
    //int passCount=fSize/DROPBOX_CHUNK_SIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath <<endl ;
        delete(req);
        return false;
    }

//    req->post(mediaData);
    req->setInputDataStream(mediaData);
    req->exec();


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job[QStringLiteral("session_id")].toString();

    if(sessID==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")<<endl  ;
        delete(req);
        return false;
    }

    delete(req);

    req=new MSHttpRequest(this->proxyServer);

    if(file.size() <= DROPBOX_CHUNK_SIZE){// onepass and finalize uploading

            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
            req->setMethod("post");
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert(QStringLiteral("session_id"),sessID);
            cursor.insert(QStringLiteral("offset"),0);
            QJsonObject commit;
            commit.insert(QStringLiteral("path"),object->path+object->fileName);
            commit.insert(QStringLiteral("mode"),QStringLiteral("overwrite"));
            commit.insert(QStringLiteral("autorename"),false);
            commit.insert(QStringLiteral("mute"),false);
            commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert(QStringLiteral("cursor"),cursor);
            doc.insert(QStringLiteral("commit"),commit);

            req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()



            req->addHeader(QStringLiteral("Content-Length"),QString::number(file.size()).toLocal8Bit());
//            req->post(file.read(fSize));
            req->setInputDataStream(&file);
            req->exec();

            file.close();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                return false;
            }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                 return false;
            }

            //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
            // This code was added for FUSE

            QString content = req->readReplyText();

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();

            utimbuf tb;
            tb.actime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;
            tb.modtime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;

            utime(filePath.toLocal8Bit().constData(),&tb);
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


            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/append"));
            req->setMethod("post");
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert(QStringLiteral("session_id"),sessID);
            cursor1->insert("offset",cursorPosition);


            req->addHeader("Dropbox-API-Arg",QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()

            file.seek(cursorPosition);


            req->addHeader(QStringLiteral("Content-Length"),QString::number(blsz).toLocal8Bit());
//            req->post(file.read(blsz));
            req->setInputDataStream(&file);
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

            delete(req);

            delete(cursor1);
            cursorPosition += blsz;

            if((unsigned long long)cursorPosition > (unsigned long long)fSize - DROPBOX_CHUNK_SIZE){

                req=new MSHttpRequest(this->proxyServer);
                //cursorPosition -= blsz;
                break;
            }

            req=new MSHttpRequest(this->proxyServer);

        }while (true);

        // finalize upload

        req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
        req->setMethod("post");
        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert(QStringLiteral("session_id"),sessID);
        cursor.insert(QStringLiteral("offset"),cursorPosition);
        QJsonObject commit;
        commit.insert(QStringLiteral("path"),object->path+object->fileName);
        commit.insert(QStringLiteral("mode"),QStringLiteral("overwrite"));
        commit.insert(QStringLiteral("autorename"),false);
        commit.insert(QStringLiteral("mute"),false);
        commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert(QStringLiteral("cursor"),cursor);
        doc.insert(QStringLiteral("commit"),commit);

        req->addHeader("Dropbox-API-Arg",QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()


        file.seek(cursorPosition);



        req->addHeader(QStringLiteral("Content-Length"),QString::number(fSize-cursorPosition).toLocal8Bit());
//        req->post(file.read(fSize-cursorPosition));
        req->setInputDataStream(&file);
        req->exec();

        file.close();

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            return false;
        }

        if(!this->testReplyBodyForError(req->readReplyText())){
            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
            return false;
        }

        //  changing a local file timestamp to timestamp of remote file to make it equals for correct SYNC status processing
        // This code was added for FUSE

        QString content = req->readReplyText();

        QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
        QJsonObject job = json.object();

        utimbuf tb;
        tb.actime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;
        tb.modtime=(this->toMilliseconds(job[QStringLiteral("client_modified")].toString(),true))/1000;;

        utime(filePath.toLocal8Bit().constData(),&tb);
        //------------------------------------------------------------------------------------------------------------------


    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob[QStringLiteral("id")].toString();

    if(rjob[QStringLiteral("id")].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote") <<endl ;
        return false;
    }

    delete(req);
    return true;
}

//=======================================================================================

bool MSDropbox::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/files/create_folder"));

    req->setMethod("post");
    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;
    metaJson.insert(QStringLiteral("path"),object->path+object->fileName);


    metaData.append((QJsonDocument(metaJson).toJson()));

//    req->post(metaData);
    req->setInputDataStream(metaData);
    req->exec();


    if(!req->replyOK()){
        if(!req->replyErrorText.contains(QStringLiteral("Conflict"))){ // skip error on re-create existing folder

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
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
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

    if(job[QStringLiteral("id")].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when folder create ")+filePath+QStringLiteral(" on remote")<<endl  ;
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

    if(this->getFlag(QStringLiteral("dryRun"))){
        return true;
    }


    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/files/delete"));


    req->setMethod("post");
    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/json; charset=UTF-8"));

    QByteArray metaData;

    QJsonObject metaJson;
    metaJson.insert(QStringLiteral("path"),object->path+object->fileName);


    metaData.append((QJsonDocument(metaJson).toJson()));

//    req->post(metaData);

    req->setInputDataStream(metaData);
    req->exec();

    if(!this->testReplyBodyForError(req->readReplyText())){
        QString errt=this->getReplyErrorString(req->readReplyText());

        if(! errt.contains(QStringLiteral("path_lookup/not_found/"))){// ignore previous deleted files

            qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
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
    if(job["id"].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when move to trash ")+filePath+QStringLiteral(" on remote") <<endl ;
    }

    delete(req);

    return true;
}

//=======================================================================================

bool MSDropbox::remote_createDirectory(const QString &path){

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






// ============= LOCAL FUNCTIONS BLOCK =============
//=======================================================================================

void MSDropbox::local_createDirectory(const QString &path){

    if(this->getFlag(QStringLiteral("dryRun"))){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================

void MSDropbox::local_removeFile(const QString &path){

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



bool MSDropbox::directUpload(const QString &url, const QString &remotePath){

    qStdOut() << "NOT INPLEMENTED YET"<<endl;
    return false;

    // download file into temp file ---------------------------------------------------------------

    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    QString tempFileName=this->generateRandom(10);
    QString filePath=this->workPath+QStringLiteral("/")+tempFileName;

    req->setMethod(QStringLiteral("get"));
    req->download(url,filePath);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        return false;
    }

    QFileInfo fileRemote(remotePath);
    QString path=fileRemote.absoluteFilePath();

    path=QString(path).left(path.lastIndexOf(QStringLiteral("/")));

    MSFSObject object;
    object.path=path+QStringLiteral("/");
    object.fileName=fileRemote.fileName();
    object.getLocalMimeType(filePath);
    object.local.modifiedDate=   this->toMilliseconds(QFileInfo(filePath).lastModified(),true);

    //this->syncFileList.insert(object.path+object.fileName,object);

    if(path!=QStringLiteral("/")){

        QStringList dirs=path.split(QStringLiteral("/"));

        MSFSObject* obj=0;
        QString cPath="/";

        for(int i=1;i<dirs.size();i++){

            obj=new MSFSObject();
            obj->path=cPath+dirs[i];
            obj->fileName=QStringLiteral("");
            obj->remote.exist=false;
            this->remote_file_makeFolder(obj);
            cPath+=dirs[i]+QStringLiteral("/");
            delete(obj);

        }


    }



    delete(req);




    // upload file to remote ------------------------------------------------------------------------



    // start session ===========

    req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/start"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));


    QByteArray mediaData;


    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    int passCount=fSize/DROPBOX_CHUNK_SIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<QStringLiteral("Unable to open of ")+filePath  ;
        delete(req);
        return false;
    }

//    req->post(mediaData);
    req->setInputDataStream(mediaData);
    req->exec();


    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString sessID=job[QStringLiteral("session_id")].toString();

    if(sessID==""){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")  ;
    }

    delete(req);

    req=new MSHttpRequest(this->proxyServer);

    if(passCount==0){// onepass and finalize uploading

            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
            req->setMethod(QStringLiteral("post"));
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert(QStringLiteral("session_id"),sessID);
            cursor.insert(QStringLiteral("offset"),0);
            QJsonObject commit;
            commit.insert(QStringLiteral("path"),remotePath);
            commit.insert(QStringLiteral("mode"),QStringLiteral("overwrite"));
            commit.insert(QStringLiteral("autorename"),false);
            commit.insert(QStringLiteral("mute"),false);
            commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object.local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

            QJsonObject doc;
            doc.insert(QStringLiteral("cursor"),cursor);
            doc.insert(QStringLiteral("commit"),commit);

            req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
//            QByteArray ba;

//            ba.append(file.read(fSize));
//            file.close();

            req->addHeader(QStringLiteral("Content-Length"),QString::number(file.size()).toLocal8Bit());
//            req->post(ba);
            req->setInputDataStream(&file);
            req->exec();

            file.close();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                QFile rfile(filePath);
                rfile.remove();
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

    }
    else{ // multipass uploading

        int cursorPosition=0;

        for(int i=0;i<passCount;i++){

            req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/append"));
            req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
            req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

            QJsonObject* cursor1=new QJsonObject();
            cursor1->insert(QStringLiteral("session_id"),sessID);
            cursor1->insert(QStringLiteral("offset"),cursorPosition);


            req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(*cursor1).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
            QByteArray* ba1=new QByteArray();

            file.seek(cursorPosition);
            ba1->append(file.read(DROPBOX_CHUNK_SIZE));


            req->addHeader(QStringLiteral("Content-Length"),QString::number(ba1->length()).toLocal8Bit());
            req->post(*ba1);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                QFile rfile(filePath);
                rfile.remove();
                return false;
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                    return false;
                }

            delete(req);
            delete(ba1);
            delete(cursor1);
            cursorPosition += DROPBOX_CHUNK_SIZE;

            req=new MSHttpRequest(this->proxyServer);
        }

        // finalize upload

        req->setRequestUrl(QStringLiteral("https://content.dropboxapi.com/2/files/upload_session/finish"));
        req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
        req->addHeader(QStringLiteral("Content-Type"),                      QStringLiteral("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert(QStringLiteral("session_id"),sessID);
        cursor.insert(QStringLiteral("offset"),cursorPosition);
        QJsonObject commit;
        commit.insert(QStringLiteral("path"),remotePath);
        commit.insert(QStringLiteral("mode"),QStringLiteral("overwrite"));
        commit.insert(QStringLiteral("autorename"),false);
        commit.insert(QStringLiteral("mute"),false);
        commit.insert(QStringLiteral("client_modified"),QDateTime::fromMSecsSinceEpoch(object.local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

        QJsonObject doc;
        doc.insert(QStringLiteral("cursor"),cursor);
        doc.insert(QStringLiteral("commit"),commit);

        req->addHeader(QStringLiteral("Dropbox-API-Arg"),QJsonDocument(doc).toJson(QJsonDocument::Compact) );//QJsonDocument(doc).toJson()
        QByteArray ba;

        file.seek(cursorPosition);
        ba.append(file.read(fSize-cursorPosition));
        file.close();

        req->addHeader(QStringLiteral("Content-Length"),QString::number(ba.length()).toLocal8Bit());
        req->post(ba);

        if(!req->replyOK()){
            req->printReplyError();
            delete(req);
            QFile rfile(filePath);
            rfile.remove();
            return false;
        }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req->readReplyText()) << endl;
                delete(req);
                return false;
            }

    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();


    if(rjob[QStringLiteral("id")].toString()==QStringLiteral("")){
        qStdOut()<< QStringLiteral("Error when upload ")+filePath+QStringLiteral(" on remote")  ;
    }

    QFile rfile(filePath);
    rfile.remove();

    delete(req);

    return true;

}

QString MSDropbox::getInfo(){


    MSHttpRequest *req0 = new MSHttpRequest(this->proxyServer);

    req0->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/users/get_current_account"));
    req0->setMethod(QStringLiteral("post"));

    req0->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req0->addHeader(QStringLiteral("Content-Type"),                     QStringLiteral(""));

//    req0->notUseContentType=true;// NEWNEW

    req0->exec();
    //req0->post(QByteArray("[]"));


    if(!req0->replyOK()){
        req0->printReplyError();

        qStdOut()<< req0->replyText<<endl;
        delete(req0);
        return QStringLiteral("false");
    }

    if(!this->testReplyBodyForError(req0->readReplyText())){
        qStdOut()<< QStringLiteral("Service error. ") << this->getReplyErrorString(req0->readReplyText()) << endl;
        delete(req0);
        return QStringLiteral("false");
    }


    QString content0=req0->readReplyText();


    QJsonDocument json0 = QJsonDocument::fromJson(content0.toUtf8());
    QJsonObject job0 = json0.object();


    delete(req0);



    MSHttpRequest *req = new MSHttpRequest(this->proxyServer);

    req->setRequestUrl(QStringLiteral("https://api.dropboxapi.com/2/users/get_space_usage"));
    req->setMethod(QStringLiteral("post"));

    req->addHeader(QStringLiteral("Authorization"),                     QStringLiteral("Bearer ")+this->access_token);
    req->addHeader(QStringLiteral("Content-Type"),                     QStringLiteral(""));

//    req->notUseContentType=true;// NEWNEW

    req->exec();


    if(!req->replyOK()){
        req->printReplyError();

        qStdOut()<< req->replyText<<endl;
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

    if(job[QStringLiteral("allocation")].toObject().empty()){
        //qStdOut()<< "Error getting cloud information "  ;
        return QStringLiteral("false");
    }

    delete(req);

    QJsonObject out;
    out[QStringLiteral("account")]=job0[QStringLiteral("email")].toString();
    out[QStringLiteral("total")]= QString::number( (uint64_t)job[QStringLiteral("allocation")].toObject()[QStringLiteral("allocated")].toDouble());
    out[QStringLiteral("usage")]= QString::number( job["used"].toInt());

    return QString( QJsonDocument(out).toJson());

}



bool MSDropbox::remote_file_empty_trash(){

    qStdOut() << "This option does not supported by this provider" << endl;
    return false;

}
