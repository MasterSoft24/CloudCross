#include "ccseparatethread.h"
#include "libfusecc.h"

//QCoreApplication* CCSeparateThread::a=NULL;
//int CCSeparateThread::argc = 1;
//char * CCSeparateThread::argv[] = {"SharedLibrary", NULL};

CCSeparateThread::CCSeparateThread(QObject *parent) : QObject(parent){

}





bool CCSeparateThread::command_getRemoteFileList(){

    MSCloudProvider* cp = lpProviderObject;
    lpFuseCC->readRemoteFileList(cp);

    return true;

}

bool CCSeparateThread::command_getFileContent(){

    QString cachePath = commandParameters["cachePath"].toString(); //this->params["params"].toObject()["cachePath"].toString();
    QString filePath = commandParameters["filePath"].toString(); //this->params["params"].toObject()["filePath"].toString();
    QString pathToCache= cachePath.replace(filePath,""); //cachePath.replace(filePath,"");


    log("getFileContent "+filePath+"   "+ pathToCache);

    QHash<QString,MSFSObject>::iterator i = lpProviderObject->syncFileList.find(filePath);

    if(i != lpProviderObject->syncFileList.end()){
        MSFSObject obj = i.value();

        MSCloudProvider* cp = lpProviderObject;

        lpFuseCC->readFileContent(cp,pathToCache,obj);


        log("getFileContent CloudProvider remote_file_get executed");
        return true;
    }

    log("getFileContent ERROR!!!!");
    return  false;
}



bool CCSeparateThread::command_doSync(){


    QString cachePath = commandParameters["cachePath"].toString();

    MSCloudProvider* cp = lpProviderObject;

    QHash<QString,MSFSObject> work;// = cp->syncFileList;


    QHash<QString,MSFSObject>::iterator i = cp->syncFileList.begin();

    for(;i != cp->syncFileList.end();i++){

        if( ((i.value().state == MSFSObject::ObjectState::ChangedLocal) ||
             (i.value().state == MSFSObject::ObjectState::NewLocal) ||
             (i.value().state == MSFSObject::ObjectState::DeleteLocal)) &&
                (i.key().indexOf(cachePath) != 0)){

            // hack fo correcting a object state
            if(i.value().state != MSFSObject::ObjectState::DeleteLocal){
                if(i.value().remote.exist == false){
                    i.value().state = MSFSObject::ObjectState::NewLocal;
                }
                else{
                    i.value().state = MSFSObject::ObjectState::ChangedLocal;
                }
            }

            // try to get parent of current object
            MSFSObject po=cp->filelist_getParentFSObject(i.value());
            if( po.path != ""){
                if( po.state != MSFSObject::ObjectState::NewLocal){

                    po.state = MSFSObject::ObjectState::Sync;
                }
                work.insert(po.path+po.fileName,po);
            }


            work.insert(i.key(),i.value());

        }

    }

    //cp->syncFileList = work;
    cp->doSync(work);

    cp->syncFileList.clear();

    lpFuseCC->readRemoteFileList(cp);
    lpFuseCC->readLocalFileList(cp,cachePath);

    return true;


}



void CCSeparateThread::run(){

    if(this->commandToExecute == "get_remote_file_list"){

        this->command_getRemoteFileList();

    }

    if(this->commandToExecute == "get_content"){

        this->command_getFileContent();

    }


    if(this->commandToExecute == "sync"){

        this->command_doSync();

    }

    emit finished();
    return ;

}



void CCSeparateThread::log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%T - ",timeinfo);

        mes = QString(buffer)+QString("SEP_THR ")+mes+" \n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }


    return;
}



