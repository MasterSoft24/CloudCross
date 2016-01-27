#include <QCoreApplication>

#include "include/msrequest.h"
#include "include/msoptparser.h"
#include "include/msproviderspool.h"
#include <QNetworkReply>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDateTime>

#define APP_MAJOR_VERSION 1
#define APP_MINOR_VERSION 0
#define APP_BUILD_NUMBER  1
#define APP_SUFFIX "-rc1"
#define APP_NAME "CloudCross"




void printHelp(){
    qDebug().nospace() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX;
    qDebug().noquote()<< QObject::tr("is a opensource application for sync local files with a Google drive cloud storage.\n");
    qDebug().noquote()<< QObject::tr("Options:");
    qDebug().noquote()<< QObject::tr("   -h [ --help ]         Produce help message");
    qDebug().noquote()<< QObject::tr("   -v [ --version ]      Display CloudCross version");
    qDebug().noquote()<< QObject::tr("   -a [ --auth ]         Request authorization token");
    qDebug().noquote()<< QObject::tr("   -p [ --path ] arg     Path to sync");
    qDebug().noquote()<< QObject::tr("   --dry-run             Only detect which files need to be uploaded/downloaded,\nwithout actually performing them.");
    qDebug().noquote()<< QObject::tr("   -s [ --list ]         Print Google drive remote file list");
    qDebug().noquote()<< QObject::tr("   --use-include         Use .include file. Without this option by default use .exclude file.\nIf these files does'nt exists, they  are ignore");
    qDebug().noquote()<< QObject::tr("   --prefer arg          Define sync strategy. It can be a one of \"remote\" or \"local\". By default it's' \"local\"");
    qDebug().noquote()<< QObject::tr("   --no-hidden           Not sync hidden files and folders");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");
//    qDebug().noquote()<< QObject::tr("");

//    qDebug().noquote()<< QObject::tr("");

}

void printVersion(){
    qDebug().nospace() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX;
}


void authGrive(MSProvidersPool* providers){

    MSRequest* req=new MSRequest();

    req->setRequestUrl("https://accounts.google.com/o/oauth2/v2/auth");
    req->setMethod("get");

    req->addQueryItem("scope",                  "https://www.googleapis.com/auth/drive+https://www.googleapis.com/auth/userinfo.email+https://www.googleapis.com/auth/userinfo.profile+https://docs.google.com/feeds/+https://docs.googleusercontent.com/+https://spreadsheets.google.com/feeds/");
    req->addQueryItem("redirect_uri",           "urn:ietf:wg:oauth:2.0:oob");
    req->addQueryItem("response_type",          "code");
    req->addQueryItem("client_id",              "502802353894-fjbma0deq577lug7hrui8ma3ogv03se2.apps.googleusercontent.com");
    req->addQueryItem("access_type",            "offline");
    req->addQueryItem("approval_prompt",        "force");
    req->addQueryItem("state",                  "1");

    req->exec();

    qDebug()<<"-------------------------------------";
    qDebug().noquote()<< QObject::tr("Please go to this URL and get an authentication code:\n");

    qDebug().noquote() << req->lastReply->url().toString();
    qDebug()<<"";
    qDebug()<<"-------------------------------------";
    qDebug().noquote()<<QObject::tr("Please input the authentication code here: ");

    QTextStream s(stdin);
    QString authCode = s.readLine();

    delete(req);

    req=new MSRequest();

    req->setRequestUrl("https://accounts.google.com/o/oauth2/token");
    req->setMethod("post");

    req->addQueryItem("client_id",          "502802353894-fjbma0deq577lug7hrui8ma3ogv03se2.apps.googleusercontent.com");
    req->addQueryItem("client_secret",      "HMQXlR2HDhrw58KR5lDQYKea");
    req->addQueryItem("code",               authCode.trimmed());
    req->addQueryItem("grant_type",         "authorization_code");
    req->addQueryItem("redirect_uri",       "urn:ietf:wg:oauth:2.0:oob");

    req->exec();

    QString content= req->lastReply->readAll();

    //qDebug() << content;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job["refresh_token"].toString();

    if(v!=""){
        MSGoogleDrive* gdp=new MSGoogleDrive();
        gdp->token=v;
        providers->addProvider(gdp);
        providers->saveTokenFile("GoogleDrive");

        qDebug().noquote() << "Token was succesfully accepted and saved. To start working with the program run grive2 without any options for start full synchronize.";
    }

}



void listGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    providers->addProvider(gdp);
    providers->loadTokenFile("GoogleDrive");

    providers->refreshToken("GoogleDrive");

    QHash<QString,MSFSObject> list=gdp->getRemoteFileList();

    QHash<QString,MSFSObject>::iterator li=list.begin();

    while(li != list.end()){
        qStdOut() << li.key()<< endl;
        li++;
    }

}


void syncGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    providers->addProvider(gdp);
    providers->loadTokenFile("GoogleDrive");

    providers->refreshToken("GoogleDrive");

    //gdp->get("", MSCloudProvider::CloudObjects::Files|MSCloudProvider::CloudObjects::NoTrash);

    //QString listRemote=gdp->filesList();

    //qDebug().noquote() <<listRemote;

    gdp->createSyncFileList();

}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);



    // create main objects

    MSProvidersPool* providers=new MSProvidersPool();


    QStringList opts=a.arguments();

    MSOptParser* parser=new MSOptParser();

    parser->insertOption(1,"--help -h  ");
    parser->insertOption(2,"-a --auth");
    parser->insertOption(3,"-v --version");
    parser->insertOption(4,"-p --path 1");
    parser->insertOption(7,"-s --list");
    parser->insertOption(8,"--use-include");

    parser->insertOption(5," --prefer 1"); // local or remote
    parser->insertOption(6,"--no-hidden");// don't sync hidden files and folders
//    parser->insertOption(9,"--new-rev");
    parser->insertOption(10,"--dry-run");

    //...............

    parser->parse(a.arguments());


    int ret;

    while((ret=parser->get())!=-1){
        switch(ret){

        case 1:
            printHelp();
            exit(0);
            //qDebug()<< "HELP arg="+parser->optarg;
            break;

        case 2:

            authGrive(providers);
            exit(0);
            break;

        case 3:
            printVersion();
            exit(0);
            break;


        case 4:
            providers->setWorkPath(parser->optarg);
            break;

        case 5:

            MSCloudProvider::SyncStrategy s;

            if(parser->optarg=="local"){
                s=MSCloudProvider::SyncStrategy::PreferLocal;
            }

            if(parser->optarg=="remote"){
                s=MSCloudProvider::SyncStrategy::PreferRemote;
            }
            else{
                qDebug().noquote()<< "--prefer option value must be an one of \"local\" or \"remote\"";
                exit(0);
                break;
            }

            providers->setStrategy(s);
            break;

        case 7:// --list

            listGrive(providers);
            exit(0);
            break;

        case 6: // --no-hidden

            providers->setFlag("noHidden",true);
            break;

        case 8: // --use-include

            providers->setFlag("useInclude",true);
            break;

        case 10: // --dry-run

            providers->setFlag("dryRun",true);
            break;

        default: // syn execute without any params by default

            syncGrive(providers);


            exit(0);
            break;
        }



    }

    if(parser->erorrNum!=0){
        qDebug()<< parser->errorString;
        exit(1);
    }


    //return a.exec();
}


/*
 *
 */














