#include "msyandexdisk.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


#define YADISK_MAX_FILESIZE  8000000
//  150000000

MSYandexDisk::MSYandexDisk() :
    MSCloudProvider()
{
    this->providerName=     "YandexDisk";
    this->tokenFileName=    ".yadisk";
    this->stateFileName=    ".yadisk_state";
    this->trashFileName=    ".trash_yadisk";
}


//=======================================================================================

bool MSYandexDisk::auth(){



    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<< QObject::tr("Please go to this URL and get an authentication code:\n")<<endl;


    qStdOut() << "https://oauth.yandex.ru/authorize?response_type=token&client_id=" << "ba0517299fbc4e6db5ec7c040baccca7&state=" << this->generateRandom(20) <<endl;

    qStdOut()<<""<<endl;
    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<<QObject::tr("Please input the authentication code here: ")<<endl;



    QTextStream s(stdin);
    QString authCode = s.readLine();

//    delete(req);

//    req=new MSRequest();

//    req->setRequestUrl("https://api.dropboxapi.com/1/oauth2/token");
//    req->setMethod("post");

//    req->addQueryItem("client_id",          "jqw8z760uh31l92");
//    req->addQueryItem("client_secret",      "eeuwcu3kzqrbl9j");
//    req->addQueryItem("code",               authCode.trimmed());
//    req->addQueryItem("grant_type",         "authorization_code");
//    //req->addQueryItem("redirect_uri",       "urn:ietf:wg:oauth:2.0:oob");

//    req->exec();


//    if(!req->replyOK()){
//        req->printReplyError();
//        return false;
//    }

////    if(!this->testReplyBodyForError(req->readReplyText())){
////        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
////        exit(0);
////    }


//    QString content= req->replyText;//lastReply->readAll();

//    //qStdOut() << content;


//    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
//    QJsonObject job = json.object();
    QString v=  authCode ;//job["access_token"].toString();

    if(v!=""){

        this->token=v;
        qStdOut() << "Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize."<<endl;
        return true;
    }
    else{
        return false;
    }

}

//=======================================================================================

QString MSYandexDisk::generateRandom(int count){

    int Low=0x41;
    int High=0x5a;
    qsrand(qrand());

    QString token="";


    for(int i=0; i<count;i++){
        qint8 d=qrand() % ((High + 1) - Low) + Low;

        if(d == 92){
           token+="\\"; // экранируем символ
        }
        else{
            token+=QChar(d);
        }
    }

    return token;

}

//=======================================================================================

void MSYandexDisk::saveTokenFile(QString path){

    QFile key(path+"/"+this->tokenFileName);
    key.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outk(&key);
    outk << "{\"access_token\" : \""+this->token+"\"}";
    key.close();

}


//=======================================================================================

bool MSYandexDisk::loadTokenFile(QString path){

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

void MSYandexDisk::loadStateFile(){

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

bool MSYandexDisk::refreshToken(){

    this->access_token=this->token;
    return true;
}

//=======================================================================================

void MSYandexDisk::createHashFromRemote(){

  //<<<<<<<<  this->readRemote();

    MSRequest* req=new MSRequest();

    req->setRequestUrl("https://api.dropboxapi.com/2/files/list_folder");
    req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));




    req->exec();


    if(!req->replyOK()){
        req->printReplyError();
        exit(1);
    }

//    if(!this->testReplyBodyForError(req->readReplyText())){
//        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
//        exit(0);
//    }


    QString list=req->readReplyText();


//    // collect all files in GoogleDrive to array driveJSONFileList
//    QJsonDocument jsonList = QJsonDocument::fromJson(list.toUtf8());
//    QJsonObject job = jsonList.object();

//    do{

//        delete(req);

//        req=new MSRequest();


//        req->setRequestUrl("https://www.googleapis.com/drive/v2/files");
//        req->setMethod("get");

//        req->addQueryItem("access_token",           this->access_token);
//        req->addQueryItem("maxResults",           "1000");

//        QString nextPageToken=job["nextPageToken"].toString();

//        QJsonArray items=job["items"].toArray();

//        for(int i=0;i<items.size();i++){

//            driveJSONFileList.insert(items[i].toObject()["id"].toString(),items[i]);

//        }

//        req->addQueryItem("pageToken",           nextPageToken);

//        req->exec();


//        if(!req->replyOK()){
//            req->printReplyError();
//            exit(1);
//        }

//        if(!this->testReplyBodyForError(req->readReplyText())){
//            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
//            exit(0);
//        }


//        list=req->readReplyText();

//        jsonList = QJsonDocument::fromJson(list.toUtf8());



//        job = jsonList.object();

//    }while(job["nextPageToken"].toString()!=""); //while(false);

    delete(req);
}


//=======================================================================================

void MSYandexDisk::readRemote(QString currentPath){ //QString parentId,QString currentPath


    MSRequest* req=new MSRequest();

    req->setRequestUrl("https://cloud-api.yandex.net:443/v1/disk/resources");
    req->setMethod("get");

    req->addHeader("Authorization",                     "OAuth "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    req->addQueryItem("limit",          "1000000");
    req->addQueryItem("offset",          "0");
    req->addQueryItem("path",          currentPath);


    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }



    QString list=req->readReplyText();

    delete(req);

    QJsonDocument json = QJsonDocument::fromJson(list.toUtf8());
    QJsonObject job = json.object();
    QJsonArray entries= job["_embedded"].toObject()["items"].toArray();

    QFileInfo fi=QFileInfo(entries[0].toObject()["path"].toString()) ;

    int hasMore=entries.size();


    do{


        for(int i=0;i<entries.size();i++){

                    MSFSObject fsObject;

                    QJsonObject o=entries[i].toObject();

                    QString yPath=o["path"].toString();
                    yPath.replace("disk:/","/");

                    fsObject.path = QFileInfo(yPath).path()+"/";

                    if(fsObject.path == "//"){
                       fsObject.path ="/";
                    }


                    // test for non first-level files


                    fsObject.remote.fileSize=  o["size"].toInt();
                    fsObject.remote.data=o;
                    fsObject.remote.exist=true;
                    fsObject.isDocFormat=false;

                    fsObject.state=MSFSObject::ObjectState::NewRemote;

                    if(this->isFile(o)){

                        fsObject.fileName=o["name"].toString();
                        fsObject.remote.objectType=MSRemoteFSObject::Type::file;
                        fsObject.remote.modifiedDate=this->toMilliseconds(o["modified"].toString(),true);
                        fsObject.remote.md5Hash=o["md5"].toString();

                     }

                     if(this->isFolder(o)){

                         fsObject.fileName=o["name"].toString();
                         fsObject.remote.objectType=MSRemoteFSObject::Type::folder;
                         fsObject.remote.modifiedDate=this->toMilliseconds(o["modified"].toString(),true);

                      }

                      if(! this->filterServiceFileNames(yPath)){// skip service files and dirs

                          continue;
                      }


                      if(this->getFlag("useInclude")){//  --use-include

                          if( this->filterIncludeFileNames(yPath)){

                              continue;
                          }
                      }
                      else{// use exclude by default

                          if(! this->filterExcludeFileNames(yPath)){

                              continue;
                          }
                      }

                      if(this->isFolder(o)){
                          this->readRemote(yPath);
                      }

                      this->syncFileList.insert(yPath, fsObject);

        }


            MSRequest* req=new MSRequest();

            req->setRequestUrl("https://cloud-api.yandex.net:443/v1/disk/resources");
            req->setMethod("get");

            req->addHeader("Authorization",                     "OAuth "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

            req->addQueryItem("limit",          "1000000");
            req->addQueryItem("offset",          QString::number(entries.size()));
            req->addQueryItem("path",          currentPath);

            req->exec();

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                exit(1);
            }



            QString list=req->readReplyText();

            delete(req);

            json = QJsonDocument::fromJson(list.toUtf8());
            job = json.object();
            entries= job["_embedded"].toObject()["items"].toArray();

            hasMore=entries.size();




    }while(hasMore > 0);


}



//=======================================================================================


bool MSYandexDisk::isFile(QJsonValue remoteObject){
    if(remoteObject.toObject()["type"].toString()=="file"){
        return true;
    }
    return false;
}

//=======================================================================================

bool MSYandexDisk::isFolder(QJsonValue remoteObject){
    if(remoteObject.toObject()["type"].toString()=="dir"){
        return true;
    }
    return false;
}

//=======================================================================================

bool MSYandexDisk::filterServiceFileNames(QString path){// return false if input path is service filename

    QString reg=this->trashFileName+"*|"+this->tokenFileName+"|"+this->stateFileName+"|.include|.exclude|~";
    QRegExp regex(reg);
    int ind = regex.indexIn(path);

    if(ind != -1){
        return false;
    }
    return true;

}

//=======================================================================================

bool MSYandexDisk::filterIncludeFileNames(QString path){// return false if input path contain one of include mask

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

bool MSYandexDisk::filterExcludeFileNames(QString path){// return false if input path contain one of exclude mask

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


void MSYandexDisk::createSyncFileList(){

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
                exit(0);
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
                exit(0);
            }
        }
    }


    qStdOut()<< "Reading remote files"<<endl;


    //this->createHashFromRemote();

    // begin create



    this->readRemote("/");// top level files and folders

    qStdOut()<<"Reading local files and folders" <<endl;

    //this->readLocal(this->workPath);

this->remote_file_get(&(this->syncFileList.values()[0]));
//this->remote_file_insert(&(this->syncFileList.values()[0]));
//this->remote_file_update(&(this->syncFileList.values()[0]));
 //this->remote_file_makeFolder(&(this->syncFileList.values()[0]));
// this->remote_createDirectory((this->syncFileList.values()[0].path+this->syncFileList.values()[0].fileName));


    this->doSync();

}

//=======================================================================================


void MSYandexDisk::readLocal(QString path){



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

                if(relPath.lastIndexOf("/")==0){
                    fsObject.path="/";
                }
                else{
                    fsObject.path=QString(relPath).left(relPath.lastIndexOf("/"))+"/";
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

}



//=======================================================================================


MSFSObject::ObjectState MSYandexDisk::filelist_defineObjectState(MSLocalFSObject local, MSRemoteFSObject remote){


    if((local.exist)&&(remote.exist)){ //exists both files

        if(local.md5Hash==remote.md5Hash){


                return MSFSObject::ObjectState::Sync;

        }
        else{

            // compare last modified date for local and remote
            if(local.modifiedDate==remote.modifiedDate){

               // return MSFSObject::ObjectState::Sync;

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


}

//=======================================================================================


void MSYandexDisk::doSync(){

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

                    qStdOut()<< obj.path<<obj.fileName <<" Forced downloading." <<endl;

                    this->remote_file_get(&obj);
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

                        qStdOut()<< obj.path<<obj.fileName <<" Forced uploading." <<endl;

                        if(obj.remote.exist){

                            this->remote_file_update(&obj);
                        }
                        else{

                            this->remote_file_insert(&obj);
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

        switch(obj.state){

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


        qStdOut()<<"Syncronization end" <<endl;

}


bool MSYandexDisk::remote_file_generateIDs(int count) {
// absolete
}

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


QHash<QString,MSFSObject>   MSYandexDisk::filelist_getFSObjectsByState(QHash<QString,MSFSObject> fsObjectList,MSFSObject::ObjectState state) {

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

bool MSYandexDisk::filelist_FSObjectHasParent(MSFSObject fsObject){
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



MSFSObject MSYandexDisk::filelist_getParentFSObject(MSFSObject fsObject){

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


void MSYandexDisk::filelist_populateChanges(MSFSObject changedFSObject){

    QHash<QString,MSFSObject>::iterator object=this->syncFileList.find(changedFSObject.path+changedFSObject.fileName);

    if(object != this->syncFileList.end()){
        object.value().local=changedFSObject.local;
        object.value().remote.data=changedFSObject.remote.data;
    }
}



bool MSYandexDisk::testReplyBodyForError(QString body) {

        QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
        QJsonObject job = json.object();

        QJsonValue e=(job["error"]);
        if(e.isNull()){
            return true;
        }

        return false;


}


QString MSYandexDisk::getReplyErrorString(QString body) {

    QJsonDocument json = QJsonDocument::fromJson(body.toUtf8());
    QJsonObject job = json.object();

    QJsonValue e=(job["description"]);

    return e.toString();

}



//=======================================================================================

// download file from cloud
void MSYandexDisk::remote_file_get(MSFSObject* object){

    if(this->getFlag("dryRun")){
        return;
    }

    MSRequest *req = new MSRequest();

    req->setRequestUrl("https://cloud-api.yandex.net/v1/disk/resources/download");
    req->setMethod("get");

    req->addHeader("Authorization",                     "OAuth "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));
    req->addQueryItem("path",                           object->path+object->fileName);

    req->exec();

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }

    QString filePath=this->workPath+object->path+object->fileName;

    QString content=req->readReplyText();


    if(this->testReplyBodyForError(req->readReplyText())){

        if(object->remote.objectType==MSLocalFSObject::Type::file){

            QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
            QJsonObject job = json.object();
            QString href=job["href"].toString();

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
            qStdOut() << "Service error. "<< this->getReplyErrorString(req->readReplyText());
        }
    }


    delete(req);

}

//=======================================================================================

void MSYandexDisk::remote_file_insert(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return;
    }

    if(this->getFlag("dryRun")){
        return;
    }


    // start session ===========

    MSRequest *req = new MSRequest();

    req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/start");
   // req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    int passCount=fSize/YADISK_MAX_FILESIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return;
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

    req=new MSRequest();

    if(passCount==0){// onepass and finalize uploading

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert("session_id",sessID);
            cursor.insert("offset",0);
            QJsonObject commit;
            commit.insert("path",object->path+object->fileName);
            commit.insert("mode","add");
            commit.insert("autorename",false);
            commit.insert("mute",false);
            commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

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
                exit(1);
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    exit(0);
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
            ba1->append(file.read(YADISK_MAX_FILESIZE));


            req->addHeader("Content-Length",QString::number(ba1->length()).toLocal8Bit());
            req->post(*ba1);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                exit(1);
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    exit(0);
                }

            delete(req);
            delete(ba1);
            delete(cursor1);
            cursorPosition += YADISK_MAX_FILESIZE;

            req=new MSRequest();
        }

        // finalize upload

        req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Type",                      QString("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert("session_id",sessID);
        cursor.insert("offset",cursorPosition);
        QJsonObject commit;
        commit.insert("path",object->path+object->fileName);
        commit.insert("mode","add");
        commit.insert("autorename",false);
        commit.insert("mute",false);
        commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

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
            exit(1);
        }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                exit(0);
            }

    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob["id"].toString();

    if(rjob["id"].toString()==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
    }

    delete(req);
}


//=======================================================================================

void MSYandexDisk::remote_file_update(MSFSObject *object){

    if(object->local.objectType==MSLocalFSObject::Type::folder){

        qStdOut()<< object->fileName << " is a folder. Skipped." <<endl;
        return;
    }

    if(this->getFlag("dryRun")){
        return;
    }


    // start session ===========

    MSRequest *req = new MSRequest();

    req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/start");
   // req->setMethod("post");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/octet-stream"));


    QByteArray mediaData;

    QString filePath=this->workPath+object->path+object->fileName;

    // read file content and put him into request body
    QFile file(filePath);

    qint64 fSize=file.size();
    int passCount=fSize/YADISK_MAX_FILESIZE;// count of 150MB blocks

    if (!file.open(QIODevice::ReadOnly)){

        //error file not found
        qStdOut()<<"Unable to open of "+filePath <<endl;
        delete(req);
        return;
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

    req=new MSRequest();

    if(passCount==0){// onepass and finalize uploading

            req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
            req->addHeader("Authorization",                     "Bearer "+this->access_token);
            req->addHeader("Content-Type",                      QString("application/octet-stream"));

            QJsonObject cursor;
            cursor.insert("session_id",sessID);
            cursor.insert("offset",0);
            QJsonObject commit;
            commit.insert("path",object->path+object->fileName);
            commit.insert("mode","overwrite");
            commit.insert("autorename",false);
            commit.insert("mute",false);
            commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

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
                exit(1);
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    exit(0);
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
            ba1->append(file.read(YADISK_MAX_FILESIZE));


            req->addHeader("Content-Length",QString::number(ba1->length()).toLocal8Bit());
            req->post(*ba1);

            if(!req->replyOK()){
                req->printReplyError();
                delete(req);
                exit(1);
            }

                if(!this->testReplyBodyForError(req->readReplyText())){
                    qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                    exit(0);
                }

            delete(req);
            delete(ba1);
            delete(cursor1);
            cursorPosition += YADISK_MAX_FILESIZE;

            req=new MSRequest();
        }

        // finalize upload

        req->setRequestUrl("https://content.dropboxapi.com/2/files/upload_session/finish");
        req->addHeader("Authorization",                     "Bearer "+this->access_token);
        req->addHeader("Content-Type",                      QString("application/octet-stream"));

        QJsonObject cursor;
        cursor.insert("session_id",sessID);
        cursor.insert("offset",cursorPosition);
        QJsonObject commit;
        commit.insert("path",object->path+object->fileName);
        commit.insert("mode","overwrite");
        commit.insert("autorename",false);
        commit.insert("mute",false);
        commit.insert("client_modified",QDateTime::fromMSecsSinceEpoch(object->local.modifiedDate,Qt::UTC).toString(Qt::DateFormat::ISODate));

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
            exit(1);
        }

            if(!this->testReplyBodyForError(req->readReplyText())){
                qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
                exit(0);
            }

    }

    QString rcontent=req->readReplyText();

    QJsonDocument rjson = QJsonDocument::fromJson(rcontent.toUtf8());
    QJsonObject rjob = rjson.object();
    object->local.newRemoteID=rjob["id"].toString();

    if(rjob["id"].toString()==""){
        qStdOut()<< "Error when upload "+filePath+" on remote" <<endl;
    }

    delete(req);
}

//=======================================================================================

void MSYandexDisk::remote_file_makeFolder(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return;
    }

    MSRequest *req = new MSRequest();

    req->setRequestUrl("https://api.dropboxapi.com/2/files/create_folder");

    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    QByteArray metaData;

    //make file metadata in json representation
    QJsonObject metaJson;
    metaJson.insert("path",object->path+object->fileName);


    metaData.append(QString(QJsonDocument(metaJson).toJson()).toLocal8Bit());

    req->post(metaData);


    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }

    if(!this->testReplyBodyForError(req->readReplyText())){
        qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
        exit(0);
    }



    QString filePath=this->workPath+object->path+object->fileName;

    QString content=req->readReplyText();

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    object->local.newRemoteID=job["id"].toString();
    object->remote.data=job;
    object->remote.exist=true;

    if(job["id"].toString()==""){
        qStdOut()<< "Error when folder create "+filePath+" on remote" <<endl;
    }

    delete(req);

    this->filelist_populateChanges(*object);

}

//=======================================================================================

void MSYandexDisk::remote_file_makeFolder(MSFSObject *object, QString parentID){
// obsolete

}

//=======================================================================================

void MSYandexDisk::remote_file_trash(MSFSObject *object){

    if(this->getFlag("dryRun")){
        return;
    }


    MSRequest *req = new MSRequest();

    req->setRequestUrl("https://api.dropboxapi.com/2/files/delete");


    req->addHeader("Authorization",                     "Bearer "+this->access_token);
    req->addHeader("Content-Type",                      QString("application/json; charset=UTF-8"));

    QByteArray metaData;

    QJsonObject metaJson;
    metaJson.insert("path",object->path+object->fileName);


    metaData.append(QString(QJsonDocument(metaJson).toJson()).toLocal8Bit());

    req->post(metaData);

    if(!this->testReplyBodyForError(req->readReplyText())){
        QString errt=this->getReplyErrorString(req->readReplyText());

        if(! errt.contains("path_lookup/not_found/")){// ignore previous deleted files

            qStdOut()<< "Service error. " << this->getReplyErrorString(req->readReplyText()) << endl;
            delete(req);
            exit(0);
        }
        else{
            delete(req);
            return;
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

}

//=======================================================================================

void MSYandexDisk::remote_createDirectory(QString path){

    if(this->getFlag("dryRun")){
        return;
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

//                MSFSObject parObj=this->filelist_getParentFSObject(f.value());
//                this->remote_file_makeFolder(&f.value(),parObj.remote.data["id"].toString());
                this->remote_file_makeFolder(&f.value());

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
}






// ============= LOCAL FUNCTIONS BLOCK =============
//=======================================================================================

void MSYandexDisk::local_createDirectory(QString path){

    if(this->getFlag("dryRun")){
        return;
    }

    QDir d;
    d.mkpath(path);

}

//=======================================================================================

void MSYandexDisk::local_removeFile(QString path){

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

void MSYandexDisk::local_removeFolder(QString path){

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

bool MSYandexDisk::local_writeFileContent(QString filePath, QString  hrefToDownload){


    MSRequest *req = new MSRequest();

    req->setRequestUrl(hrefToDownload);
    req->setMethod("get");

    req->download(hrefToDownload);

    if(!req->replyOK()){
        req->printReplyError();
        delete(req);
        exit(1);
    }


    QFile file(filePath);
    file.open(QIODevice::WriteOnly );
    QDataStream outk(&file);

    QByteArray ba;
    ba.append(req->readReplyText());

    int sz=ba.size();


    outk.writeRawData(ba.data(),ba.size()) ;

    file.close();
    return true;
}





