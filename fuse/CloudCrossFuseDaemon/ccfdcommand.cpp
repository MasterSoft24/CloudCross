#include "ccfdcommand.h"
#include "include/msproviderspool.h"

CCFDCommand::CCFDCommand(QObject *parent)
    : QObject(parent)
{




}


void CCFDCommand::log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        mes="COMMAND: "+mes+"\n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

    return;
}


void CCFDCommand::parseParameters(){

    this->command=      this->params["command"].toString();
    this->socket_name=  this->params["params"].toObject()["socket"].toString();
    this->path=         this->params["params"].toObject()["path"].toString();
    this->provider=     (ProviderType)this->params["params"].toObject()["provider"].toString().toInt();

}

QJsonObject CCFDCommand::getRetStub(){

    QJsonObject rs;
    rs["code"]=0;
    rs["result"]={};

    return rs;
}



void CCFDCommand::executor(MSCloudProvider* p){


    if(this->command == "unlink"){//                     <<<<<----- unlink

        QString filePath = this->params["params"].toObject()["filePath"].toString();

        this->workerPtr->removedItems.append(filePath);
        log("INSERT ITEM FOR REMOVE "+filePath);

    }

    //...............................................................

     if(this->command == "need_update"){//                     <<<<<----- need_update

         log("WATCHER: need shedule update");
         this->workerPtr->updateSheduled = true;
         this->workerPtr->lastUpdateSheduled = QDateTime::currentSecsSinceEpoch();

     }

    //...............................................................


    if(this->command == "get_file_list"){//                     <<<<<----- get_file_list

        p->_readRemote("");

        if(this->workerPtr != NULL){
            this->workerPtr->fileList = p->syncFileList;
        }

//        QJsonObject col;
//        QHash<QString,MSFSObject>::iterator i=p->syncFileList.begin();

//        for(;i != p->syncFileList.end();i++){

//            col[i.key()]=this->FSObjectToJSON(i.value());
//        }


        //log("readRemote "+QJsonDocument(col).toJson());
        QJsonObject r=getRetStub();
        r["code"]=0;
        r["result"]="Success";

        emit result(QJsonDocument(r).toJson());

    }

    //...............................................................

    if(this->command == "find_object"){//                       <<<<<----- find_object


        QString op = this->params["params"].toObject()["objectPath"].toString();

        QHash<QString,MSFSObject>::iterator i = this->workerPtr->fileList.find(op);

        QJsonObject r=getRetStub();

        if( i != this->workerPtr->fileList.end() ){


            r["code"] = 0;
            r["result"] = "OK";
            r["data"] = this->FSObjectToJSON(i.value());

        }
        else{

            r["code"]=-1;
            r["result"]="Object not found";

        }

        emit result(QJsonDocument(r).toJson());
    }

    //...............................................................

    if(this->command == "add_object"){//                       <<<<<----- add_object

        QString op = this->params["params"].toObject()["objectPath"].toString();

        p->syncFileList = this->workerPtr->fileList;
        p->workPath = this->workerPtr->watcherStruct->path; // path co cache

        bool res = p->readLocalSingle(op);

        this->workerPtr->fileList = p->syncFileList;

        QJsonObject r=getRetStub();
        r["code"]=0;
        r["result"]="Success";

        emit result(QJsonDocument(r).toJson());

    }

    //...............................................................

    if(this->command == "remove_object"){//                       <<<<<----- remove_object

        QString op = this->params["params"].toObject()["objectPath"].toString();

        QHash<QString, MSFSObject>::iterator i = this->workerPtr->fileList.find(op);
        QJsonObject r=getRetStub();

        if( i != this->workerPtr->fileList.end()){

            this->workerPtr->fileList.remove(op);


            r["code"]=0;
            r["result"]="Success";
        }
        else{

            QJsonObject r=getRetStub();
            r["code"]=-1;
            r["result"]="Object not found";
        }


        emit result(QJsonDocument(r).toJson());

    }


    //...............................................................

    if(this->command == "get_dir_list"){//                       <<<<<----- get_dir_list

        if(this->workerPtr != NULL){

            QString dp = this->params["params"].toObject()["dirPath"].toString();

            QJsonObject col;

            //MSFSObject obj = this->workerPtr->fileList.find(filePath).value();

            QHash<QString,MSFSObject>::iterator i = this->workerPtr->fileList.begin() ;

            for(;i != this->workerPtr->fileList.end();i++){

                if(i.value().path == dp){

                    col[i.key()]=this->FSObjectToJSON(i.value());

                }

            }

            emit result(QJsonDocument(col).toJson());

        }
        else{
            QJsonObject r=getRetStub();
            r["code"]=-1;
            r["result"]="Worker does not exists";

            emit result(QJsonDocument(r).toJson());
        }


    }


    //...............................................................


    if(this->command == "get_content"){//                       <<<<<----- get_content

        QString cachePath = this->params["params"].toObject()["cachePath"].toString();
        QString filePath = this->params["params"].toObject()["filePath"].toString();
        QString pathToCache= cachePath.replace(filePath,"");

        MSFSObject obj = this->workerPtr->fileList.find(filePath).value();
        p->workPath=pathToCache;
        p->remote_file_get(&obj);

    }

    //...............................................................

    if(this->command == "start_sync"){//                       <<<<<----- start_sync
        log("SYNCER: Started");
        //log("REMOVED_ITEMS SIZE IS "+QString::number(this->removedItems.size()));
        QString pathToCache= this->workerPtr->watcherStruct->path;

        p->_readRemote("");
        p->workPath = pathToCache;
        p->readLocal(pathToCache);


        QHash<QString,MSFSObject> copy;
        QHash<QString,MSFSObject>::iterator i = p->syncFileList.begin();

        for(;i != p->syncFileList.end();i++){

             if( this->workerPtr->removedItems.contains(i.key()) ){

                 log("INSERT DELETED ITEM "+i.value().fileName);
                 i.value().state = MSFSObject::ObjectState::DeleteLocal;
                 copy.insert(i.key(),i.value());
                 continue;
             }

            if( i.value().local.exist){

                copy.insert(i.key(),i.value());
            }

        }

        p->syncFileList.clear();
        p->syncFileList = copy;
        copy.clear();
        this->workerPtr->removedItems.clear();

        p->doSync();

        // send unlock write operations command to worker

    }


    if(this->command == "start_watcher"){//                       <<<<<----- start_watcher


        log("FS WATCHER STARTING");

            //static fswatcher_param* prm= (fswatcher_param*)param;

            static fswatcher_param* prm= new fswatcher_param;
            prm->credPath = this->params["params"].toObject()["path"].toString();
            prm->path = this->params["params"].toObject()["watchPath"].toString();

            this->workerPtr->watcherStruct = prm;

            //return ;



    }

    delete(p);
}



void CCFDCommand::run(){ //execute received command and send result back to caller through socket

    parseParameters();

    MSProvidersPool* providers=new MSProvidersPool();

    providers->workPath=this->path;



        switch (this->provider) {

        case ProviderType::Google:{

            log("GD");

            MSGoogleDrive* p_gd=new MSGoogleDrive();

            //p->setProxyServer();

            providers->addProvider(p_gd,true);

            if(!providers->loadTokenFile("GoogleDrive")){
                log("GD: Can't load token");
                return ;
            }

            if(! providers->refreshToken("GoogleDrive")){

                QJsonObject r=getRetStub();
                r["code"]=-1;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                log("GD: Can't refresh token");
                return;
            }

            executor(p_gd);

            break;
        }

        case ProviderType::Yandex:{

            MSYandexDisk* p_yd=new MSYandexDisk();

            //p->setProxyServer();

            providers->addProvider(p_yd,true);

            if(!providers->loadTokenFile("YandexDisk")){
                return ;
            }

            if(! providers->refreshToken("YandexDisk")){

                QJsonObject r=getRetStub();
                r["code"]=-1;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                return;
            }

            executor(p_yd);

            break;
        }

        case ProviderType::Mailru:{

            MSMailRu* p_mr=new MSMailRu();

            //p->setProxyServer();

            providers->addProvider(p_mr,true);

            if(!providers->loadTokenFile("MailRu")){
                return ;
            }

            if(! providers->refreshToken("MailRu")){

                QJsonObject r=getRetStub();
                r["code"]=-1;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                return;
            }

            executor(p_mr);

            break;
        }

        case ProviderType::OneDrive:{

            MSOneDrive* p_od=new MSOneDrive();

            //p->setProxyServer();

            providers->addProvider(p_od,true);

            if(!providers->loadTokenFile("OneDrive")){
                return ;
            }

            if(! providers->refreshToken("OneDrive")){

                QJsonObject r=getRetStub();
                r["code"]=-1;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                return;
            }

            executor(p_od);

            break;
        }

        case ProviderType::Dropbox:{

            MSDropbox* p_db=new MSDropbox();

            //p->setProxyServer();

            providers->addProvider(p_db,true);

            if(!providers->loadTokenFile("Dropbox")){
                return ;
            }

            if(! providers->refreshToken("Dropbox")){

                QJsonObject r=getRetStub();
                r["code"]=-1;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                return;
            }

            executor(p_db);

            break;
        }
        default:
            //#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
            log("DEFAULT");
            break;
        }



           // emit result("This is file list!! Huh\n");




        emit finished();
        return;
}


QJsonObject CCFDCommand::FSObjectToJSON(const MSFSObject& obj){

    QJsonObject o;

    o["path"]=              obj.path;
    o["fileName"]=          obj.fileName;
    o["isDocFormat"]=       obj.isDocFormat;
    o["state"]=             obj.state;

    QJsonObject local;

    local["exist"]=         obj.local.exist;
    local["modifiedDate"]=  obj.local.modifiedDate;
    local["md5Hash"]=       obj.local.md5Hash;
    local["fileSize"]=      obj.local.fileSize;
    local["mimeType"]=      obj.local.mimeType;
    local["newRemoteID"]=   obj.local.newRemoteID;
    local["objectType"]=    obj.local.objectType;

    o["local"]=             local ;


    QJsonObject remote;

    remote["exist"]=         obj.remote.exist;
    remote["modifiedDate"]=  obj.remote.modifiedDate;
    remote["md5Hash"]=       obj.remote.md5Hash;
    remote["fileSize"]=      obj.remote.fileSize;
    remote["objectType"]=    obj.remote.objectType;
    //remote["data"]=          obj.remote.data;

    o["remote"]=            remote;

    return o;
}

