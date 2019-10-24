#include "include/mssyncthread.h"

MSSyncThread::MSSyncThread(QObject *parent,MSCloudProvider* p) : QObject(parent)
{
    this->provider = p;
}

void MSSyncThread::run(){

    if(this->threadSyncList.size()>0){
        this->provider->threadsRunning->acquire(1);
        this->provider->doSync(this->threadSyncList);
        this->provider->threadsRunning->release(1);

        this->threadSyncList.clear();
    }
    emit finished();
    this->thread()->quit();
}
