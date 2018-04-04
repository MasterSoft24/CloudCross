## Change log:

### v1.4.2
- Fixed few issue (#50, #53, #54, #57)
- For Google Drive  fix a bug in updating a remote files, fix a looped uploading/downloading a converted docs, RFC3339 date conversion added.
- fixed a local deleting nested folders and files

### v1.4.1
- Fix bug with downloading files. Now files are downloads into temp files before replace existing files on disk.
- New option --no-sync. Used for uploading without synchronization and reading remote file list.
- New option --empty-trash added.  This option works only with Yandex and Google providers (for this version. In the future possible appears other providers with supporting this feature.).
- New option --filter-type was added.
- New option --low-memory added. 
- .include and .exclude files can contains now a regular expressions in Perl style or a wildcards. (thanks to [@futuarmo](https://github.com/futuarmo))
- Closed few issues #27, #26, #24, #14, #15, #22, #21, #45
- Added a multi-threaded uploading/downloading. This feature can de avoid by --single-thread option.
- Many enhancements for a reduced memory utilization.
- Remove --direct-upload option

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


