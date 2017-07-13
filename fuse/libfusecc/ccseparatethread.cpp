#include "ccseparatethread.h"
#include "libfusecc.h"


CCSeparateThread::CCSeparateThread(QObject *parent) : QObject(parent){

}





bool CCSeparateThread::command_getRemoteFileList(){

    MSCloudProvider* cp = lpProviderObject;
     lpFuseCC->readRemoteFileList(cp);
     //qDebug() << QString::number((qlonglong)cp)<< " THR";
    return true;

}

bool CCSeparateThread::command_getFileContent(){

    QString cachePath = commandParameters["cachePath"].toString(); //this->params["params"].toObject()["cachePath"].toString();
    QString filePath = commandParameters["filePath"].toString(); //this->params["params"].toObject()["filePath"].toString();
    QString pathToCache= cachePath.replace(filePath,""); //cachePath.replace(filePath,"");

    QHash<QString,MSFSObject>::iterator i = lpProviderObject->syncFileList.find(filePath);

    if(i != lpProviderObject->syncFileList.end()){
        MSFSObject obj = i.value();

        MSCloudProvider* cp = lpProviderObject;
        QString b =cp->workPath;
        cp->workPath = pathToCache;
        cp->remote_file_get(&obj);
        cp->workPath = b;

    }
     //this->workerPtr->fileList.find(filePath).value();



}



void CCSeparateThread::run(){

    if(this->commandToExecute == "get_remote_file_list"){

        this->command_getRemoteFileList();

    }

    if(this->commandToExecute == "get_content"){

        this->command_getFileContent();

    }


    emit finished();
    return ;

}
