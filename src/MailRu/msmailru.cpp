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

        }

        qStdOut() << "Token was succesfully accepted and saved."<<endl;

        this->cookies=req->manager->cookieJar();
        this->readRemote("/");
        return true;
    }
    else{
        this->providerAuthStatus=false;
        return false;
    }




}

//=======================================================================================


void MSMailRu::saveTokenFile(QString path)
{
    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"access_token\" : \""+this->token+"\", \"build\": \""+this->build+"\", \"x_page_id\": \""+this->x_page_id+"\" }";
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
//var uri = new Uri(string.Format("{0}/api/v2/folder?token={1}&home={2}", ConstSettings.CloudDomain, this.Account.AuthToken, HttpUtility.UrlEncode(path)));
    MSRequest* req=new MSRequest();

    //this->cookies=new QNetworkCookieJar();

    req->MSsetCookieJar(this->cookies);

    req->setRequestUrl("https://cloud.mail.ru/api/v2/folder");
    req->setMethod("get");

//    req->addHeader("Authorization",                     "Bearer "+this->access_token);
//    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    req->addQueryItem("token",       this->token);
    req->addQueryItem("home",       "/");
    req->addQueryItem("build",       this->build);
    req->addQueryItem("x-page-id",      this->x_page_id);
    req->addQueryItem("api",      "2");
//    req->addQueryItem("offset",      "0");
//    req->addQueryItem("limit",      "500");
    req->addQueryItem("email",      "megalion73@inbox.ru");
    req->addQueryItem("x-email",      "megalion73@inbox.ru");
    req->addQueryItem("_",      "1433249148810");

//curl 'https://cloud.mail.ru/api/v2/file?home=%2F&api=2&build=hotfix_CLOUDWEB-7114_38-0-5.201610211410&x-page-id=UC3Z9tMlKC&email=megalion73%40inbox.ru&x-email=megalion73%40inbox.ru&token=844X1uGaZ7f7MoYmqss3rJ4zx8EbXGHW&_=1478407655326' -H 'Cookie: p=Uw8AAMT7dwAA; mrcu=600856BB37C7411FAC665FC6684F; b=z0EAAAD0+VwAJwBAAAAA; searchuid=1787725681455105895; i=AQB8SL5XCAATAAg6E50AAioBAXEBAqgCAdkCAdsCAUYFAmYGAQIHAuwHAQ4KAWopAY0pAZIpAZ8pAcMpAcUpAcYpAdQpAcYACAQBAQABuwEIBAECAAFfAggKA0kAAxIIAhMIApMCCAQB3QAB3AQIBAEBAAHhBAkBAeIECgQSC7UH; VID=2iTUYz1rpKnW0000030614HW::268790694:; act=43bb08cb89444373b1a6db609f8324f9; t=obLD1AAAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAACAABILtQcA; sdcs=lMECr3d2sRTn4zmN; s=dpr=1; Mpop=1478415910:5b7c7a5242506d44190502190805001b04071d04064568515c455f0701001e0b73071e545c5f505c58585b0006105d595b5e48184a47:megalion73@inbox.ru:' -H 'DNT: 1' -H 'Accept-Encoding: gzip, deflate, sdch, br' -H 'Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.148 Safari/537.36 Vivaldi/1.4.589.38' -H 'Accept: */*' -H 'Referer: https://cloud.mail.ru/home/' -H 'X-Requested-With: XMLHttpRequest' -H 'Connection: keep-alive' --compressed

    req->exec();

     req->cookieToJSON();


    if(!req->replyOK()){
        delete(req);
        req->printReplyError();
        return false;
    }




    QString list=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = json.object();

int g=77;
}

//=======================================================================================


bool MSMailRu::readLocal(QString path)
{

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


bool MSMailRu::filterServiceFileNames(QString path)
{

}

//=======================================================================================


bool MSMailRu::filterIncludeFileNames(QString path)
{

}

//=======================================================================================


bool MSMailRu::filterExcludeFileNames(QString path)
{

}

//=======================================================================================


bool MSMailRu::createSyncFileList()
{

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
