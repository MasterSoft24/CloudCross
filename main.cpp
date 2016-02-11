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
#define APP_BUILD_NUMBER  4
#define APP_SUFFIX ""
#define APP_NAME "CloudCross"



void printHelp(){
    qStdOut() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX <<endl;
    qStdOut()<< QObject::tr("is a opensource application for sync local files with a Google drive cloud storage.\n") <<endl;
    qStdOut()<< QObject::tr("Options:") <<endl;
    qStdOut()<< QObject::tr("   -h [ --help ]         Produce help message") <<endl;
    qStdOut()<< QObject::tr("   -v [ --version ]      Display CloudCross version") <<endl;
    qStdOut()<< QObject::tr("   -a [ --auth ]         Request authorization token") <<endl;
    qStdOut()<< QObject::tr("   -p [ --path ] arg     Path to sync directory") <<endl;
    qStdOut()<< QObject::tr("   --dry-run             Only detect which files need to be uploaded/downloaded,\n"
                            "                         without actually performing them.") <<endl;
    qStdOut()<< QObject::tr("   -s [ --list ]         Print Google drive remote file list") <<endl;
    qStdOut()<< QObject::tr("   --use-include         Use .include file. Without this option by default use .exclude file.\n"
                            "                         If these files does'nt exists, they  are ignore") <<endl;
    qStdOut()<< QObject::tr("   --prefer arg          Define sync strategy. It can be a one of \"remote\" or \"local\". By default it's \"local\"") <<endl;
    qStdOut()<< QObject::tr("   --no-hidden           Not sync hidden files and folders") <<endl;
    qStdOut()<< QObject::tr("   --no-new-rev          Do not create new revisions of files, overwrite their instead") <<endl;
    qStdOut()<< QObject::tr("   --convert-doc         Convert office document to Google Doc format when upload\n"
                            "                         and convert him back when download") <<endl;
    qStdOut()<< QObject::tr("   --force arg           Forcing upload or download files. It can be a one of \"upload\" or \"download\".\n"
                            "                         This option overrides --prefer option value.") <<endl;

}

void printVersion(){
    qStdOut() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX <<endl;
}


void authGrive(MSProvidersPool* providers){

    MSRequest* req=new MSRequest();

    req->setRequestUrl("https://accounts.google.com/o/oauth2/v2/auth");
    req->setMethod("get");

    req->addQueryItem("scope",                  "https://www.googleapis.com/auth/drive+https://www.googleapis.com/auth/userinfo.email+https://www.googleapis.com/auth/userinfo.profile+https://docs.google.com/feeds/+https://docs.googleusercontent.com/+https://spreadsheets.google.com/feeds/");
    req->addQueryItem("redirect_uri",           "urn:ietf:wg:oauth:2.0:oob");
    req->addQueryItem("response_type",          "code");
    req->addQueryItem("client_id",              "834415955748-oq0p2m5dro2bvh3bu0o5bp19ok3qrs3f.apps.googleusercontent.com");
    req->addQueryItem("access_type",            "offline");
    req->addQueryItem("approval_prompt",        "force");
    req->addQueryItem("state",                  "1");

    req->exec();

    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<< QObject::tr("Please go to this URL and get an authentication code:\n")<<endl;

    qStdOut() << req->replyURL;//lastReply->url().toString();
    qStdOut()<<""<<endl;
    qStdOut()<<"-------------------------------------"<<endl;
    qStdOut()<<QObject::tr("Please input the authentication code here: ")<<endl;


    QTextStream s(stdin);
    QString authCode = s.readLine();

    delete(req);

    req=new MSRequest();

    req->setRequestUrl("https://accounts.google.com/o/oauth2/token");
    req->setMethod("post");

    req->addQueryItem("client_id",          "834415955748-oq0p2m5dro2bvh3bu0o5bp19ok3qrs3f.apps.googleusercontent.com");
    req->addQueryItem("client_secret",      "YMBWydU58CvF3UP9CSna-BwS");
    req->addQueryItem("code",               authCode.trimmed());
    req->addQueryItem("grant_type",         "authorization_code");
    req->addQueryItem("redirect_uri",       "urn:ietf:wg:oauth:2.0:oob");

    req->exec();

    QString content= req->replyText;//lastReply->readAll();

    //qStdOut() << content;

    QJsonDocument json = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject job = json.object();
    QString v=job["refresh_token"].toString();

    if(v!=""){
        MSGoogleDrive* gdp=new MSGoogleDrive();
        gdp->token=v;
        providers->addProvider(gdp,true);
        providers->saveTokenFile("GoogleDrive");

        qStdOut() << "Token was succesfully accepted and saved. To start working with the program run ccross without any options for start full synchronize."<<endl;
    }

}



void listGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    providers->addProvider(gdp,true);
    providers->loadTokenFile("GoogleDrive");

    if(! providers->refreshToken("GoogleDrive")){

        qStdOut()<< "Unauthorized client"<<endl;
        exit(0);
    }



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    QHash<QString,MSFSObject> list=gdp->getRemoteFileList();

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=list.begin();

    while(li != list.end()){
        sorted.insert(li.key(),true);
        li++;
    }

    si=sorted.begin();

    // print remote file list
    while(si != sorted.end()){
        qStdOut() << si.key()<< endl;
        si++;
    }

}


void syncGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();

    providers->addProvider(gdp);
    if(! providers->loadTokenFile("GoogleDrive")){
        exit(0);
    }

    if(!providers->refreshToken("GoogleDrive")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }



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
    parser->insertOption(9,"--no-new-rev");// do not create new revision of file, overwrite him instead
    parser->insertOption(10,"--dry-run");
    parser->insertOption(11,"--convert-doc");
    parser->insertOption(12,"--force 1");

    //...............

    parser->parse(a.arguments());


    int ret;

    while((ret=parser->get())!=-1){
        switch(ret){

        case 1: // --help

            printHelp();
            exit(0);
            //qStdOut()<< "HELP arg="+parser->optarg;
            break;

        case 2: //-- auth

            authGrive(providers);
            exit(0);
            break;

        case 3: // --version

            printVersion();
            exit(0);
            break;


        case 4: // --path

            providers->setWorkPath(parser->optarg);
            break;

        case 5: // --prefer

            MSCloudProvider::SyncStrategy s;

            if(parser->optarg=="local"){
                s=MSCloudProvider::SyncStrategy::PreferLocal;
            }

            else{
                if(parser->optarg=="remote"){
                s=MSCloudProvider::SyncStrategy::PreferRemote;
                }
                else{
                    qStdOut()<< "--prefer option value must be an one of \"local\" or \"remote\""<<endl;
                    exit(0);
                    break;
                }
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

        case 9: // --no-new-rev

            providers->setFlag("noNewRev",true);
            break;

        case 10: // --dry-run

            providers->setFlag("dryRun",true);
            break;

        case 11: // --convert-doc

            providers->setFlag("convertDoc",true);
            break;

        case 12: // --force


            if((parser->optarg=="upload")||(parser->optarg=="download")){
                providers->setOption("force",parser->optarg);
                providers->setFlag("force",true);
            }
            else{
                qStdOut()<< "--force option value must be an one of \"upload\" or \"download\""<<endl;
                exit(0);
                break;
            }
            break;


        default: // syn execute without any params by default

            syncGrive(providers);


            exit(0);
            break;
        }



    }

    if(parser->erorrNum!=0){
        qStdOut()<< parser->errorString<<endl;
        exit(1);
    }


    //return a.exec();
}













