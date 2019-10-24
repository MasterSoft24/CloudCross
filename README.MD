# The CloudCross Project

## Current version: 1.4.5

Release date: **2019-03-11**

Author: **Kamensky Vladimir**

The CloudCross is a open source project for a synchronization between your devices and various cloud storages. This software is a cross-platform and could be run on Linux, Windows and Mac OS.

CloudCross allows you to sync only a portion of the local/remote files and folders using black or white lists (.include and .exclude files).
At the same time you have the opportunity to choose which files have the advantage - local or remote. Thus, you can keep relevance either local files or files on cloud storage.

### For install on Gentoo Linux and hopefully its derivatives (Funtoo, Calculate etc.)

a  CloudCross ebuild may be found in [programmador](https://bitbucket.org/programmador/gentoo-overlay/overview) overlay.

 For simple including of overlays in Your system [Layman](https://wiki.gentoo.org/wiki/Layman) may be used:

  layman -a -s programmador

 Now CloudCross should be installable:

  emerge --ask net-misc/cloudcross

### For install on deb-based distributions (Debian,Ubuntu,Linux Mint)

 sudo apt-get install build-essential qt5-default qtbase5-dev qt5-qmake libcurl4-openssl-dev

### For install on rpm-based distributions (Redhat,CentOS, Fedora, Alt Linux)

 yum groupinstall 'Development Tools'

 yum install qt5-qtbase qt5-qtbase-devel curl-devel

### For install on  Arch Linux

 pacman -S base-devel qt5-base curl

### Then to build CloudCross from source follow the next sequence

1. Download archive.
2. Unpack it.
3. Go to unpacked folder.
4. `mkdir build`
5. `cd build`
6. `qmake ../CloudCross.pro`
7. `make`

As a result, in the build directory will be appears a three subdirectories with a result of compilation. In a *ccross-app* folder we need a **ccross** file, in a *ccross-curl-executor* we need a **ccross-curl** file. These two file is a main CloudCross executables. Copy these files to /usr/bin directory.

For detailed usage instructions see <https://cloudcross.mastersoft24.ru/usage>
