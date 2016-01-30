
CloudCross v1.0.1-rc1
Kamensky Vladimir
27.01.2016

http://cloudcross.mastersoft24.ru

CloudCross it's opensource software for syncronization of local files and folders with Google Drive.
This program was written on pure Qt, without any third-party libraries and is compatible analogue of Grive2 (https://github.com/MasterSoft24/Grive2).
But unlike Grive2 it adds several new features, fixes old algorithmic bugs and added some new :-)
The program works with Qt version 5 and above. Qt version 4.x are not supported.

CloudCross allows you to sync only a portion of the local/remote files and folders using black or white lists (.include and .exclude files).
At the same time you have the opportunity to choose which files have the advantage - local or remote. Thus, you can keep relevance either local files or files on Google Drive.


In the near future will be added:
	- possibility of record new revision, instead of overwriting the file to Google Drive.
	- Google Document support
	- build on Windows (yet questionable)
	
	
For install on deb-based distributions (Debian,Ubuntu,Linux Mint):

sudo apt-get install build-essential qt5-default qtbase5-dev qt5-qmake


For install on rpm-based distributions (Redhat,CentOS, Fedora, Alt Linux):

yum groupinstall 'Development Tools'
yum install qt5-qtbase qt5-qtbase-devel

For install on  Arch Linux:

pacman -S base-devel qt5-base


Next,basic install sequence is:

1. Download archive. 
2. Unpack it. 
3. Go to unpacked folder.
4. mkdir build
5. cd build
6. qmake-qt5 ../CloudCross.pro
7. make

As a result, in build directory will be ccross file appears
	
	
For detailed usage instructions see http://cloudcross.mastersoft24.ru/#usage	
	
	
Change log:

v1.0.1-rc1 - first pre-release version of CloudCross	


############################################################################################

CloudCross v1.0.1-rc1
Каменский Владимир
27.01.2016

http://cloudcross.mastersoft24.ru

CloudCross - это свободная программа для синхронизации локальных файлов и папок с Google Drive. 
Программа написана на чистом Qt, без каких либо сторонних библиотек и является совместимым аналогом Grive2 (https://github.com/MasterSoft24/Grive2).
Но в отличии от Grive2, в нее добавлено несколько новых функций, исправлены старые алгоритмические ошибки и добавлены новые :-)
Программа работает с Qt версии 5 и выше. Версии Qt 4.x не поддерживаются.

CloudCross позволяет синхронизировать только часть локальных/удаленных файлов и папок используя черные или белые списки (файлы .include и .exclude).
При этом у вас есть возможность выбрать какие файлы имеют преимущество - локальные или удаленные. Таким образом, вы сможете поддерживать
актуальность либо локальных файлов, либо файлов на Google Drive. 

В скором будущем будут добавлены:
	- возможность записи новой ревизии, вместо перезаписи файла на Google Drive.
	- работа с Google документами.
	- возможность сборки по Windows (пока под вопросом)
	
	
Для установки на deb-based дистрибутивы (Debian,Ubuntu,Linux Mint):

sudo apt-get install build-essential qt5-default qtbase5-dev qt5-qmake


Для установки на rpm-based дистрибутивы (Redhat,CentOS, Fedora, Alt Linux):

yum groupinstall 'Development Tools'
yum install qt5-qtbase qt5-qtbase-devel

Для установки на Arch Linux:

pacman -S base-devel qt5-base



Сборка программы состоит из нескольких простых шагов:

1. Скачайте архив. 
2. Распакуйте его. 
3. Перейдите в распакованую папку.
4. mkdir build
5. cd build
6. qmake-qt5 ../CloudCross.pro
7. make

В результате в папке build появится файл ccross


Чтобы посмотреть инструкцию по использованию прейдите на http://cloudcross.mastersoft24.ru/#usage	
	
	
Список изменений:

v1.0.1-rc1 - первая предварительная версия CloudCross	


