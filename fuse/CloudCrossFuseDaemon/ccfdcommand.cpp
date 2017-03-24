#include "ccfdcommand.h"

CCFDCommand::CCFDCommand(QObject *parent)
    : QObject(parent)
{




}

void CCFDCommand::parseParameters(){

    this->command=this->params["command"].toString();
    this->socket_name=this->params["params"].toObject()["socket"].toString();
    this->path=this->params["params"].toObject()["path"].toString();
    this->provider=(ProviderType)this->params["params"].toObject()["privider"].toInt();

}

void CCFDCommand::run(){ //execute received command and send result back to caller through socket

    parseParameters();

    if(this->command == "get_file_list"){

        switch (this->provider) {
        case ProviderType::Google:

            break;
        case ProviderType::Yandex:

            break;
        case ProviderType::Mailru:

            break;

        case ProviderType::OneDrive:

            break;
        case ProviderType::Dropbox:

            break;
        default:
            //#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
            break;
        }



    }

        emit result("This is file list!! Huh\n");
        //emit finished();
        return;
}

