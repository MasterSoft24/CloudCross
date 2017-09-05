#include "libfusecc.h"


 int libFuseCC::argc = 1;
 char * libFuseCC::argv[] = {"libFuseCC", NULL};
 QCoreApplication * libFuseCC::app = NULL;
 QThread * libFuseCC::thread = NULL;



libFuseCC::libFuseCC()
{

    if (thread == NULL)
     {
         thread = new QThread();
         connect(thread, SIGNAL(started()), this, SLOT(onStarted()), Qt::DirectConnection);
         thread->start();
         qDebug()<< "DONE fuseCC created";
     }
}


//-----------------------------------------------------------------------------------------------

bool libFuseCC::getProviderInstance(ProviderType p, MSCloudProvider **lpProvider, const QString &workPath){

    switch (p) {

    case ProviderType::Google:{

        *lpProvider = new MSGoogleDrive();

        (*lpProvider)->workPath = workPath;
        (*lpProvider)->credentialsPath = workPath;


        if(! (*lpProvider)->loadTokenFile(workPath)){

            return false;
        }

        if(! (*lpProvider)->refreshToken()){

            return false;
        }

        return true;

        break;
    }

    case ProviderType::Yandex:{

        (*lpProvider) = new MSYandexDisk();

        (*lpProvider)->workPath = workPath;
        (*lpProvider)->credentialsPath = workPath;

        if(! (*lpProvider)->loadTokenFile(workPath)){
            return false;
        }

        if(! (*lpProvider)->refreshToken()){

            return false;
        }

        return true;

        break;
    }

    case ProviderType::Mailru:{

        (*lpProvider) = new MSMailRu();

        (*lpProvider)->workPath = workPath;
        (*lpProvider)->credentialsPath = workPath;

        if(! (*lpProvider)->loadTokenFile(workPath)){
            return false;
        }

        if(! (*lpProvider)->refreshToken()){

            return false;
        }


        return true;

        break;
    }

    case ProviderType::OneDrive:{

        (*lpProvider) = new MSOneDrive();

        (*lpProvider)->workPath = workPath;
        (*lpProvider)->credentialsPath = workPath;


        if(! (*lpProvider)->loadTokenFile(workPath)){
            return false;
        }

        if(! (*lpProvider)->refreshToken()){

            return false;
        }


        return true;

        break;
    }

    case ProviderType::Dropbox:{

        (*lpProvider) = new MSDropbox();

        (*lpProvider)->workPath = workPath;
        (*lpProvider)->credentialsPath = workPath;

        if(! (*lpProvider)->loadTokenFile(workPath)){
            return false;
        }

        if(! (*lpProvider)->refreshToken()){

            return false;
        }


        return true;

        break;
    }
    default:
        //#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
        //log("DEFAULT");
        break;
    }

    return false;
}

//-----------------------------------------------------------------------------------------------

bool libFuseCC::getProviderInstance(const QString &providerName, MSCloudProvider *(*lpProvider), const QString &workPath){

    ProviderType p;

    if(providerName == "Dropbox"){
        p = ProviderType::Dropbox;
    }
    else if(providerName == "OneDrive"){
        p = ProviderType::OneDrive;
    }
    else if(providerName == "MailRu"){
        p = ProviderType::Mailru;
    }
    else if(providerName == "YandexDisk"){
        p = ProviderType::Yandex;
    }
    else if(providerName == "GoogleDrive"){
        p = ProviderType::Google;
    }

    return getProviderInstance(p,lpProvider,workPath);

}

//-----------------------------------------------------------------------------------------------

bool libFuseCC::readRemoteFileList(MSCloudProvider *p){

    bool r = p->_readRemote("");


    return r;
}


//-----------------------------------------------------------------------------------------------

bool libFuseCC::readLocalFileList(MSCloudProvider *p, const QString workPath){

    QString back = p->workPath;

    bool r = p->readLocal(workPath);

    p->workPath = back;

    return r;
}


//-----------------------------------------------------------------------------------------------


bool libFuseCC::readSingleLocalFile(MSCloudProvider *p,const QString &pathToFile, const QString workPath){

    QString back = p->workPath;
    p->workPath = workPath;
    bool r = p->readLocalSingle(pathToFile);
    p->workPath = back;
    return p;
}

//-----------------------------------------------------------------------------------------------


bool libFuseCC::readFileContent(MSCloudProvider *p, const QString &destPath,  MSFSObject obj){

            QString b =p->workPath;
            p->workPath = destPath;
            p->remote_file_get(&obj);
            p->workPath = b;

}

//-----------------------------------------------------------------------------------------------


void libFuseCC::getSeparateThreadInstance(CCSeparateThread **lpThread){

    *lpThread = new CCSeparateThread();

}

//-----------------------------------------------------------------------------------------------


void libFuseCC::runInSeparateThread(MSCloudProvider *providerInstance, const QString command, const QMap<QString, QVariant> parms){

    CCSeparateThread* thread;
    this->getSeparateThreadInstance(&thread);

    thread->commandToExecute = command;
    thread->commandParameters = parms;
    thread->lpFuseCC = this;
    thread->lpProviderObject = providerInstance;

    QThread* thr = new QThread();

    thread->moveToThread(thr);

    QObject::connect(thr,SIGNAL(started()),thread,SLOT(run()));
    QObject::connect(thread,SIGNAL(finished()),thr,SLOT(quit()));

    thr->start();

    int y=86;
}


//-----------------------------------------------------------------------------------------------


void libFuseCC::run(MSCloudProvider *providerInstance, const QString command, const QMap<QString, QVariant> parms){

    CCSeparateThread* thread;
    this->getSeparateThreadInstance(&thread);

    thread->commandToExecute = command;
    thread->commandParameters = parms;
    thread->lpFuseCC = this;
    thread->lpProviderObject = providerInstance;


    thread->run();
    delete(thread);
}

//-----------------------------------------------------------------------------------------------


void libFuseCC::clearLocalPartOfSyncFileList(MSCloudProvider *providerInstance){

    QHash<QString,MSFSObject>::iterator i = providerInstance->syncFileList.begin();
    MSLocalFSObject empty;
    QHash<QString,MSFSObject> newList;

    for(;i != providerInstance->syncFileList.end();i++){

        if((i.value().remote.exist) && (i.value().state != MSFSObject::ObjectState::DeleteLocal)){
            i.value().local = empty;
            i.value().state = MSFSObject::ObjectState::NewRemote;
            newList.insert(i.key(),i.value());
        }

    }
    providerInstance->syncFileList = newList;
}

//-----------------------------------------------------------------------------------------------



void libFuseCC::onStarted(){

    if (QCoreApplication::instance() == NULL){
           app = new QCoreApplication(argc, argv);
           app->exec();
    }
    else{
//        app = QCoreApplication::instance();
//        app->exec();
    }

}








