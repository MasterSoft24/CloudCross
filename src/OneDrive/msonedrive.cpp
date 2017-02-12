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

bool MSOneDrive::remote_file_generateIDs(int count)
{

}

void MSOneDrive::local_createDirectory(QString path)
{

}

void MSOneDrive::local_removeFile(QString path)
{

}

void MSOneDrive::local_removeFolder(QString path)
{

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






}

void MSOneDrive::saveTokenFile(QString path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"refresh_token\" : \""+this->token+"\"}";
    key.close();
}

bool MSOneDrive::loadTokenFile(QString path){

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

    QFile key(this->workPath+"/"+this->stateFileName);

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

    QFile key(this->workPath+"/"+this->stateFileName);
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

MSFSObject::ObjectState MSOneDrive::filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote)
{

}

void MSOneDrive::doSync()
{

}

QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByState(MSFSObject::ObjectState state)
{

}

QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByState(QHash<QString, MSFSObject> fsObjectList, MSFSObject::ObjectState state)
{

}

QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type)
{

}

QHash<QString, MSFSObject> MSOneDrive::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type)
{

}

bool MSOneDrive::filelist_FSObjectHasParent(MSFSObject fsObject)
{

}

MSFSObject MSOneDrive::filelist_getParentFSObject(MSFSObject fsObject)
{

}

void MSOneDrive::filelist_populateChanges(MSFSObject changedFSObject)
{

}

bool MSOneDrive::testReplyBodyForError(QString body)
{

}

QString MSOneDrive::getReplyErrorString(QString body)
{

}

bool MSOneDrive::readRemote(QString rootPath){



    MSRequest* req=new MSRequest(this->proxyServer);

    if(rootPath != ""){
        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root:"+rootPath+":/children");
    }
    else{
        req->setRequestUrl("https://api.onedrive.com/v1.0/drive/root"+rootPath+"/children");

    }
    req->setMethod("get");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
   // req->addQueryItem("expand",           "children");


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
            //qDebug()<<nl;

           //mrq->setRequestUrl(QUrl::fromPercentEncoding(job["@odata.nextLink"].toString().toUtf8()));

            //mrq->setRequestUrl("https://api.onedrive.com/v1.0/drives('me')/items('root%252F%25D0%2598%25D0%25B7%25D0%25BE%25D0%25B1%/children?$skiptoken=MjAx");
            //mrq->setRequestUrl("https://api.onedrive.com/v1.0/me/drive/root:/Изображения:/children?$skipToken=MjAx");

//            mrq->setRequestUrl("https://api.onedrive.com/v1.0/drives('me')/items('root%252F%25D0%2598%25D0%25B7%25D0%25BE%25D0%25B1%25D1%2580%25D0%25B0%25D0%25B6%25D0%25B5%25D0%25BD%25D0%25B8%25D1%258F')/children?%24skiptoken=MjAx");

//            mrq->setMethod("get");

            mrq->addHeader("Authorization",                     "Bearer "+this->access_token);
//            mrq->addQueryItem("$skiptoken",           "MjAx");

//            mrq->exec();

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

//            nl=job1["@odata.nextLink"].toString();
//                        qDebug()<<nl;

        }

    }while(hasMore);



}

bool MSOneDrive::readLocal(QString path){

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



            if(i!=this->syncFileList.end()){// if object exists in Google Drive

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

        }

        return true;



}



bool MSOneDrive::isFolder(QJsonValue remoteObject){

    QJsonValue s=remoteObject.toObject()["folder"];

    if(s != QJsonValue()){
        return true;
    }

    return false;
}



bool MSOneDrive::isFile(QJsonValue remoteObject){

    QJsonValue s=remoteObject.toObject()["file"];

    if(s != QJsonValue()){
        return true;
    }

    return false;

}



bool MSOneDrive::filterServiceFileNames(QString path)
{

}



bool MSOneDrive::filterIncludeFileNames(QString path)
{

}



bool MSOneDrive::filterExcludeFileNames(QString path)
{

}



bool MSOneDrive::directUpload(QString url, QString remotePath)
{

}

bool MSOneDrive::onAuthFinished(QString html, MSCloudProvider *provider){

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
