## CloudCross FUSE Project

ATTENTION: This project have an ==alpha status==. You must be careful when  you'll worked with it.


Project have a two parts:
1. CloudCross FUSE library
2. CloudCross Fuse worker

- Library is needed to worker for build. 
- Build libfusecc first then build CloudCrossFuseWorker. 
- Move assembled binaries into correspond directories (/bin and /lib)

To start mount operation enter

	ccfw 2 /path/to/provider/credentials /path/to/mounpoint
 
 where 2 is a provider code
 
	// Providers code
	// 0 - Google
	// 1 - Dropbox
	// 2 - Yandex
	// 3 - Mail
	// 4 - OneDrive
	
Provider credentials you can get by running of **ccross -a**