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

void CCFDCommand::run(){ //execute received command and send result back to caller through socket

    parseParameters();

    MSProvidersPool* providers=new MSProvidersPool();

    providers->workPath=this->path;



        switch (this->provider) {
        case ProviderType::Google:

            break;
        case ProviderType::Yandex:

            break;
        case ProviderType::Mailru:

            break;

        case ProviderType::OneDrive:

            MSOneDrive* p=new MSOneDrive();
            //p->setProxyServer();

            providers->addProvider(p,true);

            if(!providers->loadTokenFile("OneDrive")){
                return ;
            }

            if(! providers->refreshToken("OneDrive")){

                QJsonObject r=getRetStub();
                r["code"]=0;
                r["result"]="Unauthorized client";

                emit result(QJsonDocument(r).toJson());
                return;
            }

            if(this->command == "get_file_list"){//                     <<<<<----- get_file_list

                p->readRemote("");

                if(this->workerPtr != NULL){
                    this->workerPtr->fileList = p->syncFileList;
                }

                QJsonObject col;
                QHash<QString,MSFSObject>::iterator i=p->syncFileList.begin();

                for(;i != p->syncFileList.end();i++){

                    col[i.key()]=this->FSObjectToJSON(i.value());
                }


                //log("readRemote "+QJsonDocument(col).toJson());
                emit result(QJsonDocument(col).toJson());
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

                QString pathToCache= this->workerPtr->watcherStruct->path;//   this->params["params"].toObject()["mount"].toString().replace("\n","");

                p->readRemote("");
                p->workPath = pathToCache;
                p->readLocal(pathToCache);


                QHash<QString,MSFSObject> copy;
                QHash<QString,MSFSObject>::iterator i = p->syncFileList.begin();

                for(;i != p->syncFileList.end();i++){

                    if( i.value().local.exist){

                        copy.insert(i.key(),i.value());
                    }

                }

                p->syncFileList.clear();
                p->syncFileList = copy;
                copy.clear();

                p->doSync();

                // send unlock write operations command to worker

            }


            if(this->command == "start_watcher"){//                       <<<<<----- start_watcher


                log("FS WATCHER STARTING");

                    //static fswatcher_param* prm= (fswatcher_param*)param;

                    static fswatcher_param* prm= new fswatcher_param;
                    prm->credPath = this->params["params"].toObject()["path"].toString();
                    prm->path = this->params["params"].toObject()["watchPath"].toString();

                    static QStringList backmem;

                    static char buffer[BUF_LEN];

                    //sleep(3);

                    prm->wd = inotify_init1(IN_NONBLOCK);

                    if(prm->wd < 0){
                        log("INOTIFY: "+QString(strerror(errno)));
                        return ;
                    }



                    log("FS WATCHER ready for watch on "+prm->path);
                    prm->watcher = inotify_add_watch( prm->wd, prm->path.toStdString().c_str(), IN_CLOSE_WRITE | IN_DELETE | IN_CREATE | IN_OPEN | IN_MOVE | IN_CLOSE_NOWRITE);

                    if(prm->watcher < 0){
                        log("INOTIFY: add watch error "+QString(strerror(errno)));
                        close(prm->wd);
                        return ;
                    }


                    this->workerPtr->watcherStruct = prm;

                    while (true) {

                        int len;

                        struct pollfd pfd = { prm->wd, POLLIN, 0 };

                        int ret = poll(&pfd, 1, 50);

                        if(ret < 0){
                            log("FS WATCHER error reading");
                            close(prm->wd);
                          return ;
                        }

                        if(ret == 0){

                            //log("FS WATCHER nothing to do");
                            continue;
                        }
                        else{

//                            log("FS WATCHER Something read now!!!!!");
                            len = read( prm->wd, &buffer[0], BUF_LEN );

                            if (len < 0) {
                                close(prm->wd);
                                inotify_rm_watch(prm->wd, prm->watcher);
                                log("FS WATCHER error reading");
                              return ;
                            }
                        }


                        int i=0;

                        while(i < len) {
                            struct inotify_event* event = (struct inotify_event*) &buffer[i];

                            if(event->len) {

                                if( (event->mask & IN_CREATE) ||
                                    (event->mask & IN_OPEN) ){

                                    //backmem.push_back(string(&event->name[0]));
                                    backmem.append(QString(&event->name[0]));

                                    log("WATCHER: need worker lock");
                                    this->workerPtr->workerLocked = true;

                                    i += EVENT_SIZE + event->len;

                                    continue;
                                }

                                if( (event->mask & IN_DELETE) ||
                                    (event->mask & IN_CLOSE_WRITE) ||
                                    (event->mask & IN_MOVED_TO)||
                                    (event->mask & IN_MOVED_FROM)){

                                    if(backmem.size() == 0){

                                        log("WATCHER: need shedule update");
                                        this->workerPtr->updateSheduled = true;
                                        this->workerPtr->lastUpdateSheduled = QDateTime::currentSecsSinceEpoch();
                                        i += EVENT_SIZE + event->len;
                                        continue;
                                    }


                                    QStringList::iterator b = backmem.begin();
                                    for(;b != backmem.end();b++){

                                        QString ename = QString(&event->name[0]);
                                        if(*b == ename){
                                            backmem.erase(b);

                                            if(backmem.size() == 0){

                                                i += EVENT_SIZE + event->len;
                                                continue;
                                            }


                                            this->workerPtr->workerLocked = false;
                                            log("WATCHER: need worker unlock");

                                            b = backmem.begin();
                                            continue;
                                            //break;
                                        }
                                    }


                                    log("WATCHER: need shedule update");
                                    this->workerPtr->updateSheduled = true;
                                    this->workerPtr->lastUpdateSheduled = QDateTime::currentSecsSinceEpoch();

                                }
                            }

                            if( (event->mask & IN_CLOSE_NOWRITE)){

                                if(backmem.size() == 0){

                                    i += EVENT_SIZE + event->len;
                                    continue;
                                }

                                QStringList::iterator b = backmem.begin();
                                for(;b != backmem.end();b++){

                                    QString ename = QString(&event->name[0]);
                                    if(*b == ename){
                                        backmem.erase(b);

                                        if(backmem.size() == 0){

                                            i += EVENT_SIZE + event->len;
                                            continue;
                                        }


                                        b = backmem.begin();
                                        log("WATCHER: need worker unlock");
                                        this->workerPtr->workerLocked = false;
                                        continue;
                                        //break;
                                    }
                                }

                            }

                            i += EVENT_SIZE + event->len;
                        }

                    }

            }

            break;

//        case ProviderType::Dropbox:

//            break;
//        default:
//            //#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
//            break;
        }



           // emit result("This is file list!! Huh\n");




        emit finished();
        return;
}

//void CCFDCommand::watcherRemovedSlot()
//{

//}

//void CCFDCommand::watcherModifiedSlot()
//{

//}

//void CCFDCommand::watcherCreatedSlot()
//{

//}

void CCFDCommand::watcherOnFileChanged(QString file)
{
log(" WATCHER: FILE CHANGED "+file);
}

void CCFDCommand::watcherOnDirectoryChanged(QString dir)
{
log(" WATCHER: DIR CHANGED "+dir);
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
    remote["data"]=          obj.remote.data;

    o["remote"]=            remote;

    return o;
}

