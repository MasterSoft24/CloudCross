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
}

//=======================================================================================


bool MSMailRu::auth(){


    MSRequest* req=new MSRequest();

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


    QString r=req->replyText;

    int start =  r.indexOf("\"csrf\"");
    QString r_out;

    delete(req);

    if(start > 0){

        start=start+8;
        r_out= r.mid(start,32);
        this->providerAuthStatus=true;
        this->token=r_out;
        qStdOut() << "Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize."<<endl;
        return true;
    }
    else{
        this->providerAuthStatus=false;
        return false;
    }




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



void MSMailRu::saveTokenFile(QString path)
{

}

//=======================================================================================


bool MSMailRu::loadTokenFile(QString path)
{

}

//=======================================================================================


void MSMailRu::loadStateFile()
{

}

//=======================================================================================


void MSMailRu::saveStateFile()
{

}

//=======================================================================================


bool MSMailRu::refreshToken()
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


bool MSMailRu::readRemote()
{

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
