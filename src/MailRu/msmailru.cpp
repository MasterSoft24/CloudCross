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


#define DROPBOX_MAX_FILESIZE  8000000


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


    MSRequest* req=new MSRequest();

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

    req->cookieToJSON();

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

        qStdOut() << "Token was succesfully accepted and saved."<<endl;

        //this->cookies=new QNetworkCookieJar(req->manager->cookieJar());
        this->cookies=(req->manager->cookieJar());

        //this->readRemote("/");

        //delete(req);
        this->providerAuthStatus=true;
        return true;
    }
    else{
        this->providerAuthStatus=false;
        delete(req);
        return false;
    }




}

//=======================================================================================


void MSMailRu::saveTokenFile(QString path)
{
    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"access_token\" : \""+this->token+"\", \"build\": \""+this->build+"\", \"x_page_id\": \""+this->x_page_id+"\", \"login\": \""+this->login+"\", \"password\": \""+this->password+"\" }";
    key.close();
}

//=======================================================================================


bool MSMailRu::loadTokenFile(QString path)
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

    QFile key(this->workPath+"/"+this->stateFileName);
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




bool MSMailRu::remote_file_get(MSFSObject *object)
{

}

//=======================================================================================


bool MSMailRu::remote_file_insert(MSFSObject *object)
{

}

//=======================================================================================


bool MSMailRu::remote_file_update(MSFSObject *object)
{

}

//=======================================================================================


bool MSMailRu::remote_file_generateIDs(int count)
{

}

//=======================================================================================


bool MSMailRu::remote_file_makeFolder(MSFSObject *object)
{

}

//=======================================================================================


void MSMailRu::remote_file_makeFolder(MSFSObject *object, QString parentID)
{

}

//=======================================================================================


bool MSMailRu::remote_file_trash(MSFSObject *object)
{

}


//=======================================================================================

bool MSMailRu::remote_createDirectory(QString path)
{

}

//=======================================================================================


void MSMailRu::local_createDirectory(QString path)
{

}

//=======================================================================================


void MSMailRu::local_removeFile(QString path)
{

}

//=======================================================================================


void MSMailRu::local_removeFolder(QString path)
{

}

//=======================================================================================


MSFSObject::ObjectState MSMailRu::filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote)
{

}

//=======================================================================================


void MSMailRu::doSync()
{

}

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByState(MSFSObject::ObjectState state)
{

}

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByState(QHash<QString, MSFSObject> fsObjectList, MSFSObject::ObjectState state)
{

}

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByTypeLocal(MSLocalFSObject::Type type)
{

}

//=======================================================================================


QHash<QString, MSFSObject> MSMailRu::filelist_getFSObjectsByTypeRemote(MSRemoteFSObject::Type type)
{

}

//=======================================================================================


bool MSMailRu::filelist_FSObjectHasParent(MSFSObject fsObject)
{

}

//=======================================================================================


MSFSObject MSMailRu::filelist_getParentFSObject(MSFSObject fsObject)
{

}

//=======================================================================================


void MSMailRu::filelist_populateChanges(MSFSObject changedFSObject)
{

}

//=======================================================================================


bool MSMailRu::testReplyBodyForError(QString body)
{

}

//=======================================================================================


QString MSMailRu::getReplyErrorString(QString body)
{

}

//=======================================================================================


bool MSMailRu::createHashFromRemote()
{

}


//=======================================================================================


bool MSMailRu::readRemote(QString path)
{

    this->auth();

    if(this->providerAuthStatus == false){
        return providerAuthStatus;
    }

    MSRequest* req_prev;

    MSRequest* req=new MSRequest();

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("https://cloud.mail.ru/api/v2/folder");
    req->setMethod("get");

    req->addQueryItem("token",       this->token);
    req->addQueryItem("home",       "/");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("x-page-id",      this->x_page_id);
    req->addQueryItem("api",      "2");
    req->addQueryItem("offset",      "0");
    req->addQueryItem("limit",      "2000000");
    req->addQueryItem("email",      this->login);
    req->addQueryItem("x-email",      this->login);
    req->addQueryItem("_",      "1433249148810");


    req->exec();

    this->cookies=  req->manager->cookieJar();


    if(!req->replyOK()){
        delete(req);
        req->printReplyError();
        return false;
    }

    QString list=req->readReplyText();


    //delete(req);
    req_prev = req;

    QVector<QJsonObject> stack;
    QJsonArray entries;


    do{

        QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
        QJsonObject job = json.object();



        entries=job["body"].toObject()["list"].toArray();


        for(int i=0; i < entries.size(); i++){// file processing circle

            QJsonObject o=entries[i].toObject();

            if(o["type"] == "folder"){
                stack.push_back(o);
                continue;
            }

            MSFSObject fsObject;

            fsObject.path = QFileInfo(o["home"].toString()).path()+"/";

            if(fsObject.path == "//"){
               fsObject.path ="/";
            }

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


        if(stack.size() >0){

            QJsonObject nl=stack.first();
            stack.pop_front();



            MSFSObject fsObject;

            fsObject.path = QFileInfo(nl["home"].toString()).path()+"/";

            if(fsObject.path == "//"){
               fsObject.path ="/";
            }

            fsObject.remote.fileSize=  nl["size"].toInt();
            fsObject.remote.data=nl;
            fsObject.remote.exist=true;
            fsObject.isDocFormat=false;
            //fsObject.remote.md5Hash=nl["hash"].toString();

            fsObject.state=MSFSObject::ObjectState::NewRemote;

            fsObject.fileName=nl["name"].toString();
            fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
            double zz=nl["mtime"].toDouble();
            QDateTime zzd=QDateTime::fromTime_t(zz);
            fsObject.remote.modifiedDate=this->toMilliseconds(zzd,true);

            if(this->getFlag("useInclude")){//  --use-include

                if( this->filterIncludeFileNames(nl["home"].toString())){

                    continue;
                }
            }
            else{// use exclude by default

                if(! this->filterExcludeFileNames(nl["home"].toString())){

                    continue;
                }
            }

            this->syncFileList.insert(nl["home"].toString(), fsObject);


            req=new MSRequest();

            req->MSsetCookieJar(this->cookies);



            req->setRequestUrl("https://cloud.mail.ru/api/v2/folder");
            req->setMethod("get");

            req->addQueryItem("token",       this->token);
            req->addQueryItem("home",       nl["home"].toString());
            req->addQueryItem("build",       this->build);
            req->addQueryItem("x-page-id",      this->x_page_id);
            req->addQueryItem("api",      "2");
            req->addQueryItem("offset",      "0");
            req->addQueryItem("limit",      "2000000");
            req->addQueryItem("email",      this->login);
            req->addQueryItem("x-email",      this->login);
            req->addQueryItem("_",      "1433249148810");


            req->exec();

            this->cookies=req->manager->cookieJar();


            if(!req->replyOK()){
                delete(req);
                req->printReplyError();
                return false;
            }

            list=req->readReplyText();

            delete(req_prev);

           //delete(req);
            req_prev = req;


        }
        else{
            break;
        }





    }while(entries.size() >= 0);


    delete(req_prev);
}

//=======================================================================================


bool MSMailRu::readLocal(QString path){


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


bool MSMailRu::isFolder(QJsonValue remoteObject)
{

}

//=======================================================================================


bool MSMailRu::isFile(QJsonValue remoteObject)
{

}

//=======================================================================================


bool MSMailRu::filterServiceFileNames(QString path){

    QString reg=this->trashFileName+"*|"+this->tokenFileName+"|"+this->stateFileName+"|.include|.exclude|~";
    QRegExp regex(reg);
    int ind = regex.indexIn(path);

    if(ind != -1){
        return false;
    }
    return true;
}

//=======================================================================================


bool MSMailRu::filterIncludeFileNames(QString path){

    if(this->includeList==""){
        return true;
    }

    // catch paths with  beginning masks from include/exclude lists
    bool isBegin=false;
    bool start;
    QRegularExpression regex3(path);
    regex3.patternErrorOffset();
    QRegularExpressionMatch m1= regex3.match(this->includeList);

    if(m1.hasMatch()){
        start=m1.capturedStart();

        if((this->includeList.mid(start-1,1)=="|") ||(start==0)){
            isBegin=true;
        }
    }

    QRegularExpression regex2(this->includeList);

    regex2.patternErrorOffset();

    QRegularExpressionMatch m = regex2.match(path);

    if(m.hasMatch()){
        return false;
    }
    else{
        if(isBegin){
            return false;
        }
        else{
            return true;
        }
    }
}

//=======================================================================================


bool MSMailRu::filterExcludeFileNames(QString path){

    if(this->excludeList==""){
        return true;
    }

    // catch paths with  beginning masks from include/exclude lists
    bool isBegin=false;
    bool start;
    QRegularExpression regex3(path);
    regex3.patternErrorOffset();
    QRegularExpressionMatch m1= regex3.match(this->excludeList);

    if(m1.hasMatch()){
        start=m1.capturedStart();

        if((this->excludeList.mid(start-1,1)=="|") ||(start==0)){
            isBegin=true;
        }
    }


    QRegularExpression regex2(this->excludeList);

    regex2.patternErrorOffset();

    QRegularExpressionMatch m = regex2.match(path);

    if(m.hasMatch()){
        return false;
    }
    else{
        if(isBegin){
            return false;
        }
        else{
            return true;
        }
    }


}

//=======================================================================================


bool MSMailRu::createSyncFileList(){

    if(this->getFlag("useInclude")){
        QFile key(this->workPath+"/.include");

        if(key.open(QIODevice::ReadOnly)){

            QTextStream instream(&key);
            QString line;
            while(!instream.atEnd()){

                QString line=instream.readLine();
                if(line.isEmpty()){
                        continue;
                }

                this->includeList=this->includeList+line+"|";
            }
            this->includeList=this->includeList.left(this->includeList.size()-1);

            QRegularExpression regex2(this->includeList);

            if(regex2.patternErrorOffset()!=-1){
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

                QString line=instream.readLine();
                if(line.isEmpty()){
                        continue;
                }
                this->excludeList=this->excludeList+line+"|";
            }
            this->excludeList=this->excludeList.left(this->excludeList.size()-1);

            QRegularExpression regex2(this->excludeList);

            if(regex2.patternErrorOffset()!=-1){
                qStdOut()<<"Exclude filelist contains errors. Program will be terminated.";
                return false;
            }
        }
    }


    qStdOut()<< "Reading remote files"<<endl;


    //this->createHashFromRemote();

    // begin create



    if(!this->readRemote("/")){// top level files and folders

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


    this->doSync();

    return true;
}

//=======================================================================================


bool MSMailRu::directUpload(QString url, QString remotePath)
{

}

//=======================================================================================


// not used
bool MSMailRu::onAuthFinished(QString html, MSCloudProvider *provider)
{

    return true;

}
