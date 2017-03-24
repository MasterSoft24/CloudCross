#include "commandserver.h"
#include <iostream>

#include <stdio.h>


#define BR "^"
#define DAEMON_MSG_END "\n<---CCFD_MESSAGE_END-->"


void log(QString mes);

CommandServer::CommandServer(QObject *parent) : QObject(parent)
{
    srv=new QLocalServer(this);

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

    QLocalSocket* sender=(QLocalSocket*)QObject::sender();


    QStringList sndParms;

    QString incomingMess=sender->readAll();

    log("DAEMON: new raw command - "+incomingMess);

    QStringList comm=incomingMess.split("^");

    if(incomingMess.contains("{")){

        log("DAEMON: was received command from worker");

        // possibly it is JSON
        QJsonDocument json = QJsonDocument::fromJson(incomingMess.toUtf8());
        QJsonObject job = json.object();

        if(job.size() == 0){// nop. isn't JSON

            //return;
        }

        QThread* thread=new QThread();
        CCFDCommand* ccfdc=new CCFDCommand();

        ccfdc->params=job;
        ccfdc->socket=sender;

        ccfdc->moveToThread(thread);

        connect(thread,SIGNAL(started()),ccfdc,SLOT(run()));
        connect(ccfdc, SIGNAL(finished()), thread, SLOT(quit()));
        connect(ccfdc,SIGNAL(result(QString)),this,SLOT(onThreadResult(QString)));

        thread->start();

        return ;
    }


    if(comm[0] == "new_mount"){

        fuse_worker* w=new fuse_worker;

        //QFile("/tmp/ccfw-test.sock").remove();

        w->worker=          new QProcess(this);
        w->worker_soket=    new QLocalServer(this);
        w->socket_name=     QString("ccfd_w_")+this->generateRandom(4)+QString(".sock");



        sndParms << w->socket_name;
        sndParms << comm[1];
        sndParms << comm[2];
        sndParms << comm[3];

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
    sender->deleteLater();
}



void CommandServer::onThreadResult(const QString &r){

    CCFDCommand* sender=(CCFDCommand*)QObject::sender();

    log("DAEMON: ready to send - "+r);

    if(!sender->socket->isOpen()){
        log ("DAEMON: socket not open");

        return;
    }
    else{

        log ("DAEMON: socket becomes to "+sender->socket->fullServerName());
    }


    qint64 sz=sender->socket->write((r+DAEMON_MSG_END).toLocal8Bit());
    if(sz == -1){
        log("DAEMON: result sending error");
        return;
    }
    sender->socket->flush();
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
