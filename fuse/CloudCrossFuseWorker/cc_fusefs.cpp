#include "cc_fusefs.h"


CC_FuseFS* CC_FuseFS::_instance = NULL;
QString CC_FuseFS::mountPath = "";
QString CC_FuseFS::tokenPath = "";
ProviderType CC_FuseFS::provider = (ProviderType)0;
QString CC_FuseFS::providerName = "";
libFuseCC* CC_FuseFS::ccLib = NULL;
MSCloudProvider* CC_FuseFS::providerObject = NULL;



CC_FuseFS::CC_FuseFS(){



}


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


CC_FuseFS* CC_FuseFS::Instance() {
    if(_instance == NULL) {
        _instance = new CC_FuseFS();
    }
    return _instance;
}




int CC_FuseFS::Open(const char *path, fuse_file_info *fileInfo){

    log("OPEN ENTERED ");
//    QCoreApplication* ca;
//    int argc = 1;
//    char * argv[] = {"CC_FuseFS", NULL};
    if (QCoreApplication::instance() == NULL){

//        ca =new QCoreApplication(argc,argv);
//        ca->exec();
        log("APP INSTANCE NOT DEFINED");
    }

//    connect(this,SIGNAL(testSig()),this,SLOT(onTestSig()));

//    emit testSig();
    //QCoreApplication::processEvents();



//    MSRequest *req = new MSRequest();
//    req->setMethod("get");

//    //req->addHeader("Authorization","Bearer "+this->access_token);

//    req->download("https://mastersoft24.ru/img/applications.png","/tmp/qqwweerr");

//    //QString c=req->readReplyText();

//    log("OPEN EXITED");

    return 0;

}

void CC_FuseFS::onTestSig(){

    log("TEST SIG RECIVED");
}





