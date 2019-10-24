/*
    CloudCross: Opensource program for syncronization of local files and folders with Google Drive

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation version 2
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
    MSOptParser - command line parameters parser class

    Copyright (C) 2016  Vladimir Kamensky
    Copyright (C) 2016  Master Soft LLC

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation version 2
    of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "include/msoptparser.h"
#include <QStringList>



/*
 *  MSOptParser - command line parameters parser class
 *
 *  Usage:
 *
 int main(int argc, char *argv[])
 {
    QCoreApplication a(argc, argv)

    MSOptParser* parser=new MSOptParser(); // create class instance

    // describe all available program options

    parser->insertOption(1,"--help -h  ");
    parser->insertOption(2,"-v --version");
    parser->insertOption(3,"-p --path 1"); // one parameter required for this option

    parser->parse(a.arguments());// parse command line parameters

    int ret;

    while((ret=parser->get())!=-1){// command line pagameters processing loop
        switch(ret){

        case 1:
            qDebug()<< "HELP";
            break;

        case 2:

            qDebug()<< "AUTH";
            break;

        case 3:

            qDebug()<< "PATH "+parser->optarg;// optarg contain option parameter
            break;
        }
    }

    if(parser->erorrNum!=0){
        qDebug()<< parser->errorString;
        exit(1);
    }

  // PROGRAM BODY
  }
 */



MSOptParser::MSOptParser(QObject *parent) : QObject(parent)
{


}


void MSOptParser::insertOption(int num, const QString &optString){

    optItem item;
    QStringList list=optString.split(" ",QString::SkipEmptyParts);

    item.num=num;


    for(int i=0;i<list.size();i++){

        QString o=list[i];

        bool ok;

        QString(o.trimmed()).toInt(&ok);

        if(ok){
            item.paramCount=QString(o.trimmed()).toInt();
            continue;
        }

        if(QString(o.trimmed().at(1))!=("-")){
            item.shortOpt=o;
            continue;
        }

        if(QString(o.trimmed().at(1))==("-")){
            item.longOpt=o;
            continue;
        }
    }


    this->opts.append(item);
}


void MSOptParser::parse(const QStringList &list){

    for(int i=0;i<list.size();i++){
        QString co=list[i];
        if(co.contains("=")){
            QStringList o=co.split("=");
            this->input.append(o[0]);
            this->input.append(o[1]);
        }
        else{
            this->input.append(co);
        }
    }

    this->iit=this->input.begin();
    this->iit++;
}


QString MSOptParser::getShort(const QStringList &list){

    if(QString(list[0].trimmed().at(1))!=("-")){
        return list[0];
    }
    return QString();
}


QString MSOptParser::getLong(const QStringList &list){

    if((QString(list[0].trimmed()).size()<3)&&(QString(list[1].trimmed()).size()<3)){
        return "";
    }

    if(QString(list[0].trimmed().at(1))==("-")){
        return list[0];
    }

    if(QString(list[1].trimmed().at(1))==("-")){
        return list[1];
    }
    return QString();
}


int MSOptParser::getParamCount(const QStringList &list){

    bool ok;

    QString(list[1].trimmed()).toInt(&ok);

    if(ok){
        return QString(list[1].trimmed()).toInt();
    }

    QString(list[2].trimmed()).toInt(&ok);

    if(ok){
        return QString(list[2].trimmed()).toInt();
    }
    return -1;
}

int MSOptParser::get(){


      int i=0;
      QString currOpt;

      this->optarg.clear();

      for(;i<this->opts.size();i++){

          if(this->iit==this->input.end()){
//              this->erorrNum=2;
//              this->errorString="!!!Unknown option "+currOpt;
              return 0;
          }

          try{
          currOpt = *this->iit;
          }
          catch(...){
              this->erorrNum=2;
              this->errorString="Unknown option "+currOpt;
              return -1;
          }




          if((currOpt==this->opts[i].shortOpt)||(currOpt==this->opts[i].longOpt)){


              if(this->opts[i].paramCount!=0){

                  for(int p=0;p<this->opts[i].paramCount;p++){

                      if(this->iit==this->input.end()){
                          this->erorrNum=1;
                          this->errorString="Option "+currOpt+" Missing required argument";
                          return -1;
                      }

                      if(this->getArg()){
                        this->optarg.append(*this->iit);

                      }
                      else{
                          this->erorrNum=1;
                          this->errorString="Option "+currOpt+" Missing required argument";
                          return -1;
                      }
                      //*this->iit++;
                  }

              }
              else{
                    this->optarg.clear();
              }
              this->iit++;
              return this->opts[i].num;
          }

      }
      this->erorrNum=2;
      this->errorString="Unknown option "+currOpt;
      return -1;
}


bool MSOptParser::getArg(){

    if((this->iit+1)==this->input.end()){
        return false;
    }

    QString a=*(this->iit+1);
    if(QString(a.trimmed().at(0))==("-")){
        return false;
    }

    this->iit++;
    return true;

}


QStringList MSOptParser::getParamByName(const QString &paramName){

    QStringList p;

    for(int i=0;i<this->input.size();i++){

        if((this->input.at(i) == paramName) || (this->input.at(i) == ("--"+paramName)) || (this->input.at(i) == ("-"+paramName))){


            optItem opi;

            // find corresponding option at list
            for(int opt=0;opt<this->opts.size();opt++){

                opi=this->opts.at(opt);
                if((opi.longOpt == this->input.at(i)) || (opi.shortOpt == this->input.at(i))  ){
                    break;
                }

            }


            if(i+opi.paramCount > this->input.size() -1){
                p.clear();
                this->iit=this->input.begin();
                this->iit++;
                return p;
            }


            //collect option parameters
            for(int pc=0;pc<opi.paramCount;pc++){

                if(QString(this->input.at(i+pc+1).at(0))!="-"){
                    p.append(this->input.at(i+pc+1));
                }
                else{
                    p.clear();
                    this->iit=this->input.begin();
                    this->iit++;
                    return p;
                }
            }

            // remove given option with all their parameters
            for(int pc=0;pc<opi.paramCount+1;pc++){
                this->input.removeAt(i);
            }


        }


    }

    this->iit=this->input.begin();
    this->iit++;
    return p;

}


bool MSOptParser::isParamExist(const QString &paramName){

    for(int i=0;i<this->input.size();i++){

        if((this->input.at(i) == paramName) || (this->input.at(i) == ("--"+paramName))){
            return true;
        }

    }
    return false;
}











