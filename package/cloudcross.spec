Summary:            CloudCross is a improved multi-cloud (Google, Dropbox, OneDrive, Yandex.Disk, Cloud Mail.ru) client with Google Docs support and many other features
Name:               cloudcross
Version: 1.4.1_rc2
Release: 1
License:            BSD
#Group:              System Environment/Base
#Source: cloudcross-v1.3.0-2.tar.gz
Source: CloudCross-master.tar.gz
#Source: dev.tar.gz
#Source: https://github.com/MasterSoft24/CloudCross/archive/v1.3.0.tar.gz
URL:                https://cloudcross.mastersoft24.ru


%if 0%{?mageia}
%ifarch x86_64 amd64
BuildRequires:      lib64proxy-webkit
BuildRequires:      curl-devel
BuildRequires:      lib64qt5base5-devel 
%else
BuildRequires:      libproxy-webkit
BuildRequires:      curl-devel
BuildRequires:      libqt5base5-devel 
%endif
%else
%if 0%{?suse_version} > 1130
BuildRequires:  libqt5-qtbase-devel
BuildRequires:      curl-devel

%else


BuildRequires:      qt5-qtbase 
BuildRequires:      qt5-qtbase-devel
BuildRequires:      curl-devel

%endif
%endif







%description
CloudCross is a improved multi-cloud (Google, Dropbox, OneDrive, Yandex.Disk, Cloud Mail.ru)  sync client with Google Docs support and many other features

%prep

#exec yum  install -y epel-release && yum -y update
#exec yum  install -y qt5-qtbase qt5-qtbase-devel



#tar -xf /home/abuild/rpmbuild/SOURCES/CloudCross-master.tar.gz
tar -xf /home/abuild/rpmbuild/SOURCES/dev.tar.gz




%build

cd CloudCross-master
#cd CloudCross-dev
%if 0%{?mageia}
qmake CloudCross.pro
%else
qmake-qt5 CloudCross.pro
%endif
make 

%install

cd CloudCross-master
#cd CloudCross-dev


%if 0%{?suse_version} > 1130
mkdir -p %{buildroot}/usr/bin
%else

mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/share/man/man0
%endif



%if 0%{?mageia}

cp ccross-app/ccross %{buildroot}/usr/bin
cp ccross-curl-executor/ccross-curl %{buildroot}/usr/bin

%else

%if 0%{?suse_version} > 1130
cp ccross-app/ccross %{buildroot}/usr/bin
cp ccross-curl-executor/ccross-curl %{buildroot}/usr/bin
#cp ccross-app/doc/ccross %{buildroot}/usr/share/man/man0
%else
cp ccross-app/ccross %{buildroot}/usr/bin
cp ccross-curl-executor/ccross-curl %{buildroot}/usr/bin
cp ccross-app/doc/ccross %{buildroot}/usr/share/man/man0
%endif
%endif

%files


%if 0%{?mageia}

%defattr(-,root,root,-)
/usr/bin/ccross
/usr/bin/ccross-curl

%else

%if 0%{?suse_version} > 1130
%defattr(-,root,root,-)
/usr/bin/ccross
/usr/bin/ccross-curl
%else
%defattr(-,root,root,-)
/usr/bin/ccross
/usr/bin/ccross-curl
/usr/share/man/man0/ccross.gz
%endif
%endif

#%if 0%{?suse_version} > 1130
#/usr/bin/ccross
#%else
#/usr/share/man/man0/ccross.gz
#%endif
