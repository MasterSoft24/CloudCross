#include "libfusecc.h"


libFuseCC::libFuseCC()
{
    qDebug()<< "fuseCC created";
}


//-----------------------------------------------------------------------------------------------

bool libFuseCC::getProviderInstance(ProviderType p, MSCloudProvider **lpProvider, const QString &workPath){

    switch (p) {

    case ProviderType::Google:{

        *lpProvider = new MSGoogleDrive();

        (*lpProvider)->workPath = workPath;


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

    return p->_readRemote("");

}


//-----------------------------------------------------------------------------------------------

bool libFuseCC::readLocalFileList(MSCloudProvider *p){

    return p->readLocal(p->workPath);

}


//-----------------------------------------------------------------------------------------------


bool libFuseCC::readSingleLocalFile(MSCloudProvider *p,const QString &path){

    return p->readLocalSingle(path);

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








