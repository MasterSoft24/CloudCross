#include "commandserver.h"
#include <iostream>

#include <stdio.h>


#define BR "^"
#define DAEMON_MSG_END "\n<---CCFD_MESSAGE_END-->"


void log(QString mes);

CommandServer::CommandServer(QObject *parent) : QObject(parent)
{
    srv = new QLocalServer(this);

    workersController = new QTimer(this);
    workersController->setInterval(60 * 1000);
    connect(workersController,SIGNAL(timeout()),this,SLOT(onWorkersProcessLoop()));

    workersController->start();
}

bool CommandServer::start(){ // start a main daemon listener

    QFile("/tmp/ccfd.sock").remove();

    bool r= srv->listen("ccfd.sock");
    if(r){
        connect(srv,SIGNAL(newConnection()),this,SLOT(onNewConnection()));
    }
    return r;
}

void CommandServer::onNewConnection(){

    QLocalServer* srvr=(QLocalServer*)QObject::sender();

    QLocalSocket *clientConnection = srvr->nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected()),this, SLOT(onClientDisconnected()));
    connect(clientConnection,SIGNAL(readyRead()),this,SLOT(onNewCommandRecieved()));

    clientConnection->write("CloudCross Fuse daemon: HELLO\n");
    clientConnection->flush();

    log("DAEMON: new connection");

    QHash<QString,fuse_worker*>::iterator i=workersList.begin();
    for(;i != workersList.end();i++){
        if(i.value()->worker_soket == srvr){
            log("DAEMON: worker was found in list");
            i.value()->clientConnection=clientConnection;
        }
    }
}


void log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        mes+="\n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

    return;
}

void CommandServer::onNewCommandRecieved(){

    QLocalSocket* sender = (QLocalSocket*)QObject::sender();


    QStringList sndParms;

    QString incomingMess = sender->readAll();

    log("DAEMON: new raw command - "+incomingMess);

    QStringList comm = incomingMess.split("^");

    if(incomingMess.contains("{")){// any command in JSON formated style

        log("DAEMON: was received command from worker");

        // possibly it is JSON
        QJsonDocument json = QJsonDocument::fromJson(incomingMess.toUtf8());
        QJsonObject job = json.object();

        if(job.size() == 0){// nop. isn't JSON

            return;
        }

        QHash<QString,fuse_worker*>::iterator i=workersList.begin();
        for(;i != workersList.end();i++){

            if(i.value()->socket_name == job["params"].toObject()["socket"].toString()){

                break;
            }
        }

        // <<<<<<<<<<< here is a potentially point of denied (need control a situation with no workers finded)

//        if(job["command"].toString() == QString("set_worker_locked")){
//            log("DAEMON: worker locked");
//            i.value()->workerLocked = true;
//            return;
//        }

//        if(job["command"].toString() == QString("set_worker_unlock")){
//            log("DAEMON: worker unlock");
//            i.value()->workerLocked = false;
//            return;
//        }

//        if(job["command"].toString() == QString("shedule_update")){
//            log("DAEMON: sedule update");
//            i.value()->updateSheduled = true;
//            i.value()->lastUpdateSheduled = QString(job["params"].toObject()["time"].toString()).toInt();
//            return;
//        }




        QThread* thread = new QThread();
        CCFDCommand* ccfdc = new CCFDCommand();

        ccfdc->params = job;
        ccfdc->socket = sender;

        if(i!= workersList.end()){
            ccfdc->workerPtr = i.value();
        }
        else{
            ccfdc->workerPtr = NULL;
        }

        ccfdc->moveToThread(thread);

        connect(thread,SIGNAL(started()),ccfdc,SLOT(run()));
        connect(ccfdc, SIGNAL(finished()), thread, SLOT(quit()));
        connect(ccfdc,SIGNAL(result(QString)),this,SLOT(onThreadResult(QString)));

        thread->start();

        return ;
    }


    if(comm[0] == "new_mount"){

        fuse_worker* w = new fuse_worker;

        //QFile("/tmp/ccfw-test.sock").remove();

        w->workerLocked = false;
        w->updateSheduled = false;

        w->worker =          new QProcess(this);
        w->worker_soket =    new QLocalServer(this);
        w->socket_name =     QString("ccfd_w_")+this->generateRandom(4)+QString(".sock");
        w->mountPoint = QString(comm[3]).replace("\n","");
        w->provider = comm[1];
        w->workPath = comm[2];

        log("DAEMON MOUNT POINT IS "+w->mountPoint);

        sndParms << w->socket_name;
        sndParms << comm[1];
        sndParms << comm[2];
        sndParms << w->mountPoint;

        // at this point a <comm> variable contains
        // [0]= socket name for communicate with command server
        // [1]= provider code
        // [2]= path to credentials
        // [3]= mount point

        QString pathTOWorkerExe="/home/ms/QT/CloudCross/fuse/build-CloudCrossFuseWorker-Desktop-Debug/CloudCrossFuseWorker";

        //comm[0]=pathTOWorkerExe;

        bool r= w->worker_soket->listen(w->socket_name);

        if(r){
            connect(w->worker_soket,SIGNAL(newConnection()),this,SLOT(onNewConnection()));

            this->workersList.insert(comm[3],w);

            w->worker->start(pathTOWorkerExe,sndParms);

        }


    }

    return;
}

void CommandServer::onClientDisconnected(){
    QLocalSocket* sender=(QLocalSocket*)QObject::sender();

    QHash<QString,fuse_worker*>::iterator i=workersList.begin();
    for(;i != workersList.end();i++){

        if(i.value()->clientConnection == sender){

            delete(i.value()->clientConnection);//->deleteLater();
            i.value()->fileList.clear();
            delete(i.value()->worker_soket);//->deleteLater();

            if(i.value()->worker->state() != QProcess::Running){

                i.value()->worker->kill();
            }

            workersList.remove(i.key());
            break;
        }
    }

   //sender->deleteLater();
}



void CommandServer::onThreadResult(const QString &r){

    CCFDCommand* sender=(CCFDCommand*)QObject::sender();

    log("DAEMON: ready to send - ");

    if(!sender->socket->isOpen()){


        return;
    }
    else{


    }


    qint64 sz=sender->socket->write((r+DAEMON_MSG_END).toLocal8Bit());
    if(sz == -1){
        log("DAEMON: result sending error");
        return;
    }
    sender->socket->flush();
}




void CommandServer::onWorkersProcessLoop(){

    log("PROCESS LOOP");
    QHash<QString,fuse_worker*>::iterator i = this->workersList.begin();

    int ct = QDateTime::currentSecsSinceEpoch();

    for(;i != this->workersList.end();i++){

        fuse_worker* w = i.value();

        if( (w->updateSheduled) && (!w->workerLocked) && ((ct - w->lastUpdateSheduled) > 20)  ){

            // start sync for current worker

            // send command to worker for block any write operation until sync ended

            // perpare data for start sync thread


            log("DAEMON: Start Sync");
            QThread* thread = new QThread();
            CCFDCommand* ccfdc = new CCFDCommand();

            QJsonObject job;
            job["command"] = QString("start_sync");

            QJsonObject pr;
            pr["socket"] =      w->socket_name;
            pr["path"] =        w->workPath;
            pr["provider"] =    w->provider;
            pr["mount"] =       w->mountPoint;

            job["params"] = pr;


            ccfdc->params = job;
            ccfdc->socket = w->clientConnection;

            ccfdc->workerPtr = w;

            ccfdc->moveToThread(thread);

            connect(thread,SIGNAL(started()),ccfdc,SLOT(run()));
            connect(ccfdc, SIGNAL(finished()), thread, SLOT(quit()));
            connect(ccfdc,SIGNAL(result(QString)),this,SLOT(onThreadResult(QString)));

            thread->start();

            w->updateSheduled = false;
        }
    }

}



QString CommandServer::generateRandom(int count){

    int Low=65;
    int High=91;
    qsrand(QDateTime::currentMSecsSinceEpoch());
    //qsrand(qrand());

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
