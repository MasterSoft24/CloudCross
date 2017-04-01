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

