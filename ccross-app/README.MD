
# CloudCross v1.4.1

Kamensky Vladimir

15.03.2017


https://cloudcross.mastersoft24.ru



CloudCross it's opensource software for synchronization of local files and folders with various cloud storages.
On this moment CloudCross supports working with Yandex Disk, Google Drive, Dropbox, OneDrive and Cloud Mail.ru.
This program was written on pure Qt, without any third-party libraries.
The program works with Qt version 5 and above. Qt version 4.x are not supported. 

CloudCross allows you to sync only a portion of the local/remote files and folders using black or white lists (.include and .exclude files).
At the same time you have the opportunity to choose which files have the advantage - local or remote. Thus, you can keep relevance either local files or files on cloud storage.

In addition, CloudCross supports automatic bidirectional conversion of documents from MS Office or Open Office format to Google Docs.
Also, you can control the versioning of files synchronized on Google Drive. 

	
	
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
6. qmake ../CloudCross.pro
7. make

As a result, in build directory will be ccross file appears
	
	
For detailed usage instructions see https://cloudcross.mastersoft24.ru/#usage
	
	
## Change log:

### v1.4.1
- New option --filter-type was added.
- .include and .exclude files can contains now a regular expressions in Perl style or a wildcards. (thanks to [@futuarmo](https://github.com/futuarmo))
- Closed few issues #27, #26, #24, #14, #15
- Added a multi-threaded uploading/downloading.
- Many enhancements for a reduced memory utilization.

### v1.4.0
- OneDrive support added.
- Showing total and free space  of clouds

### v1.3.1
- Fix some Mail.Ru working errors
- Remove necessity for definition login and password  for each Mail.ru operation. Now it needed only for authentication

### v1.3.0
- Cloud Mail.ru support added.
- Added options for proxy servers using (--http-proxy and --socks5-proxy)

### v1.2.4
- Change authentication mechanism to authenticate with using local listener.
- Added statistic collecting.
- Changed license type to BSD.

### v1.2.3
- Change  path for authorization on Google Drive.
- Change/fix random generator algorithm.
- Some minor refactoring.

### v1.2.2
- Possibility building and working on Windows.
- Fix build error on certain systems on wrong implicit conversion from char* to QString in QJsonValue::insert() method.
- Fix build warning messages.
- Fix working with Google Drive access token without lifetime limit.

### v1.2.1
- --direct-upload option added. This option allows you upload files from url's directly to cloud.

### v1.2.0
- Yandex Disk support added

### v1.1.0
- Dropbox support added
- --force​ option behaviour corrected
		
### v1.0.4
- Added an option --force to force a download or upload files and folders.

### v1.0.3
- Enhance working with .include and .exclude lists

### v1.0.2
- Added --no-new-rev option for server side versioning control
- Added --convert-doc option for auto conversion from office formats (MS/Open) to Google Doc and back.

### v1.0.1-rc3
- fixed bag with threads count overhead. fixed bag with corruption of downloaded files

### v1.0.1-rc2
- bugs fixed

### v1.0.1-rc1
- first pre-release version of CloudCross









# CloudCross v1.4.1

Каменский Владимир

15.03.2017

https://cloudcross.mastersoft24.ru/ru


CloudCross - это свободная программа для синхронизации локальных файлов и папок с различными облачными хранилищами.
На данный момент, пока, поддерживается работа с Яндекс.Диск, Google Drive, Dropbox, OneDrive и Облако Mail.ru.
Программа написана на чистом Qt, без каких либо сторонних библиотек.
CloudCross работает с Qt версии 5 и выше. Версии Qt 4.x не поддерживаются. 

CloudCross позволяет синхронизировать только часть локальных/удаленных файлов и папок используя черные или белые списки (файлы .include и .exclude).
При этом у вас есть возможность выбрать какие файлы имеют преимущество - локальные или удаленные. Таким образом, вы сможете поддерживать
актуальность либо локальных файлов, либо файлов в облаке. 

Кроме того, CloudCross поддерживает автоматическую двунаправленную конвертацию документов из формата MS Office или Open Office в формат Google Docs.
Так же, доступно управление версионированием синхронизируемых файлов на Google Drive. 

	
	
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
6. qmake ../CloudCross.pro
7. make

В результате в папке build появится файл ccross


Чтобы посмотреть инструкцию по использованию зайдите на https://cloudcross.mastersoft24.ru/#usage
	
	
## История изменений:

### v1.4.1
- Добавлена опция --filter-type.
- .include и .exclude файлы теперь могут содержать не только wildcards но и регулярки в Perl - стиле. (thanks to [@futuarmo](https://github.com/futuarmo))
- Закрыты несколько багов #27, #26, #24, #14, #15
- Добавлена возможность многопоточной загрузки/выгрузки файлов.
- Множество исправлений и улучшени для уменьшения использования памяти.

### v1.4.0
- Добавлена поддержка OneDrive
- Добавлена возможность просмотра свободного/занятого объема в облаке.

### v1.3.1
- Исправлены некоторые ошибки возникающие при работе с Mail.Ru
- Исключена необходимость указывать логин и пароль для каждой операции с Mail.Ru. Теперь это необходимо только при аутентификации.

### v1.3.0
- Добавлена поддержка облака Mail.ru.
- Добавлены опции для использования прокси-серверов(--http-proxy и --socks5-proxy)

### v1.2.4
- Изменен  старый механизм аутентификации на аутентификацию с использованием редиректа на локалхост.
- Добавлен сбор статистики.
- Изменена лицензия на BSD.

### v1.2.3
- Изменен путь для OAuth2 авторизации в Google Drive.
- Изменен агоритм генерации случайных последовательностей.
- Небольшой рефакторинг и косметические изменения.

### v1.2.2
- Реализована возможность сборки и работы под Windows.
- Исправлена ошибка сборки на некоторый системах, связаная с неявным преобразованием из char* в QString в методе QJsonValue::insert().
- Исправлен код генерирующий предупреждения  при сборке.
- Исправлена работа с токеном Google Drive без ограничения по времени жизни.

### v1.2.1
- Добавлена опция --direct-upload для прямой загрузке файла в облако по ссылке

### v1.2.0
- Добавлена поддержка Яндекс Диск

### v1.1.0
- Добавлена поддержка "облачного провайдера" Dropbox
- Скорректирована работа опции --force​

### v1.0.4
- Добавлена опция --force для принудительного скачивания или выгрузки файлов и папок.

### v1.0.3
- Улучшена обработка списков .include и .exclude

### v1.0.2
- Добавлена опция --no-new-rev для управления версионированием файлов на сервере
- Добавлена опция --convert-doc для автоматической конвертации файлов из формата офиса (MS/Open) в формат Google Doc и обратно.

### v1.0.1-rc3
- Исправлена ошибка превышения количества потоков на процесс. Испрвлена ошибка с повреждением скачиваемых файлов

### v1.0.1-rc2
- Исправление некоторых ошибок

### v1.0.1-rc1
- первый предварительный релиз CloudCross


