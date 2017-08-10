#include "cc_fusefs.h"


void* onSyncTimer(void* p);


CC_FuseFS* CC_FuseFS::_instance = NULL;
QString CC_FuseFS::mountPath = "";
QString CC_FuseFS::tokenPath = "";
//QString CC_FuseFS::credentialsPath = "";
ProviderType CC_FuseFS::provider = (ProviderType)0;
QString CC_FuseFS::providerName = "";
libFuseCC* CC_FuseFS::ccLib = NULL;
MSCloudProvider* CC_FuseFS::providerObject = NULL;
//std::thread CC_FuseFS::syncTimer = std::thread(CC_FuseFS::onSyncTimer);

bool CC_FuseFS::fsBlocked =false;
bool CC_FuseFS::updateSheduled = false;
int CC_FuseFS::lastUpdateSheduled =0;
//-------------------------------------------------------------------------

void m_log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%T - ",timeinfo);

        mes = QString(buffer)+"CC_FUSE_FS "+mes+" \n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

//    string ns="echo "+mes+" >> /tmp/ccfw.log ";
//    system(ns.c_str());
    return;
}




CC_FuseFS::CC_FuseFS(){

//    this->fsBlocked =false;
//    this->updateSheduled = false;
//    this->lastUpdateSheduled =0;
    //this->syncTimer = new QTimer();
    //connect(CC_FuseFS::syncTimer,SIGNAL(timeout()),this,SLOT(onSyncTimer()));
    //this->syncTimer->setInterval(30 * 1000);
   // CC_FuseFS::Instance()->syncTimer= std::thread(CC_FuseFS::onSyncTimer);
//    pthread_create(&(tid), NULL, &onSyncTimer, NULL);

}

//-------------------------------------------------------------------------

void CC_FuseFS::log(QString mes){

    FILE* lf=fopen("/tmp/ccfw.log","a+");
    if(lf != NULL){

        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%T - ",timeinfo);

        mes = QString(buffer)+"CC_FUSE_FS "+mes+" \n";
        fputs(mes.toStdString().c_str(),lf);
        fclose(lf);
    }

//    string ns="echo "+mes+" >> /tmp/ccfw.log ";
//    system(ns.c_str());
    return;
}

//-------------------------------------------------------------------------

void* onSyncTimer2(void* p){

    //CC_FuseFS::Instance()->log("TIMER elapsed");

    while(true){

         usleep (1000 * 3000);

         m_log("TIMER ticked");

//        int ct = QDateTime::currentSecsSinceEpoch();

//        if( (CC_FuseFS::updateSheduled) && (!CC_FuseFS::fsBlocked) && ((ct - CC_FuseFS::lastUpdateSheduled) > 20)  ){

//            CC_FuseFS::fsBlocked = true;

//            CC_FuseFS::Instance()->log("SYNC started");

//            QMap<QString,QVariant> p;

//            //CC_FuseFS::Instance()->ccLib->run(CC_FuseFS::Instance()->providerObject,"sync", p);

//            CC_FuseFS::updateSheduled = false;
//            CC_FuseFS::fsBlocked = false;
//        }

    }

}

//-------------------------------------------------------------------------

CC_FuseFS* CC_FuseFS::Instance() {
    if(_instance == NULL) {
        _instance = new CC_FuseFS();

    }
    return _instance;
}


//-------------------------------------------------------------------------

void CC_FuseFS::doSheduleUpdate(){

    CC_FuseFS::Instance()->updateSheduled = true;
    CC_FuseFS::Instance()->lastUpdateSheduled = QDateTime::currentSecsSinceEpoch();

}

//-------------------------------------------------------------------------








