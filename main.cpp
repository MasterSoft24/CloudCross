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

    qStdOut()<< QObject::tr("   --provider arg        Set cloud provider for current sync operation. On this moment this option can be \n"
                            "                         a \"google\" or \"dropbox\". Provider by default is Google Drive") <<endl;

}

void printVersion(){
    qStdOut() << APP_NAME << " v"<<APP_MAJOR_VERSION<<"."<<APP_MINOR_VERSION<<"."<<APP_BUILD_NUMBER<<APP_SUFFIX <<endl;
}


void authGrive(MSProvidersPool* providers){

    MSGoogleDrive* gdp=new MSGoogleDrive();
    if(gdp->auth()){

        providers->addProvider(gdp,true);
        providers->saveTokenFile("GoogleDrive");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
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



void authDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();
    if(dbp->auth()){

        providers->addProvider(dbp,true);
        providers->saveTokenFile("Dropbox");
    }
    else{
       qStdOut() << "Authentication failed"<<endl;
    }
}



void listDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    providers->addProvider(dbp,true);
    providers->loadTokenFile("Dropbox");

    if(! providers->refreshToken("Dropbox")){

        qStdOut()<< "Unauthorized client"<<endl;
        exit(0);
    }

    dbp->readRemote();



    QMap<QString,bool>sorted;
    QMap<QString,bool>::iterator si;

    // sort remote file list
    QHash<QString,MSFSObject>::iterator li=dbp->syncFileList.begin();

    while(li != dbp->syncFileList.end()){
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


void syncDropbox(MSProvidersPool* providers){

    MSDropbox* dbp=new MSDropbox();

    providers->addProvider(dbp);
    if(! providers->loadTokenFile("Dropbox")){
        exit(0);
    }

    if(!providers->refreshToken("Dropbox")){
        qStdOut()<<"Unauthorized access. Aborted."<<endl;
        exit(0);
    }

    dbp->createSyncFileList();

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
    parser->insertOption(13,"--provider 1"); // google or dropbox

    //...............

    parser->parse(a.arguments());


    int ret;

    QString currentProvider="google";

    QString prov=parser->getParamByName("provider");

    if(prov != ""){
        currentProvider=prov;
    }


    if((currentProvider != "google")&&(currentProvider != "dropbox")){
        qStdOut()<< "Unknown cloud provider. Application terminated."<< endl;
        return 1;
    }


    // ===%%%% GOOGLE DRIVE %%%%===

    if(currentProvider == "google"){

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
    }



    // ===%%%% DROPBOX %%%%===

    if(currentProvider == "dropbox"){

        while((ret=parser->get())!=-1){
            switch(ret){

            case 1: // --help

                printHelp();
                exit(0);
                //qStdOut()<< "HELP arg="+parser->optarg;
                break;

            case 2: //-- auth

                authDropbox(providers);
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

                listDropbox(providers);
                exit(0);
                break;

            case 6: // --no-hidden

                providers->setFlag("noHidden",true);
                break;

            case 8: // --use-include

                providers->setFlag("useInclude",true);
                break;

            case 9: // --no-new-rev

                qStdOut()<< "--no-new-rev option doesn't matter for Dropbox provider. "<<endl;
                //providers->setFlag("noNewRev",true);
                break;

            case 10: // --dry-run

                providers->setFlag("dryRun",true);
                break;

            case 11: // --convert-doc
                  qStdOut()<< "--no-new-rev option doesn't matter for Dropbox provider. "<<endl;
//                providers->setFlag("convertDoc",true);
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

                syncDropbox(providers);
//                qStdOut()<< "sync dropbox"<<endl;

                exit(0);
                break;
            }



        }


        if(parser->erorrNum!=0){
            qStdOut()<< parser->errorString<<endl;
            exit(1);
        }
    }

    //return a.exec();
}













