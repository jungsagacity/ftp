#ifndef __UPLOAD_H__
#define __UPLOAD_H__

#include <stdio.h>
#include <unistd.h>

#include "utility.h"

/********  MACRO DEFINITION  ********/
#define     TEMP_SUFFIX             ".inmp"
#define     INOTIFY_EVENTS          IN_CLOSE_WRITE|IN_DELETE
//events of monitor
#define     BUF_MAX                 1024
// the max size of buffer to store the temporary monitor events
#define     UNIX_Z                  ".Z"
//the suffix of compression
#define     PRE_FILENAME_SIZE       4
//the size of the prefix of filename ,like"ACC","ACI"..
#define     POST_FILENAME_SIZE      8
//the size of the suffix of filename ,like".irt.Z"
#define     STD_FILENAME_SIZE       20
//the max size of the standard filename ,like"ACUwwwwd_HR.sp3.Z"
#define     COMMAND_SIZE            1000
//to storre compression command,include full path and filename
#define     FILE_DIR_SIZE           7
//the week or the year&month folder
#define     BACKUP_FULL_PATH_SIZE   200
#define     BEIDOU_YEAR             2006
#define     BEIDOU_MONTH            1
#define     BEIDOU_DAY              1
//the first day of the BEIDOU week
#define     GPS_WEEK                1356
#define     PATH_SUFFIX             "/"

#define     BD                      1
#define     GPS                    2

/********  UploadNode->state  MACRO DEFINITION ********/
#define        UPLOAD_FILE_EXIST                0
//the production of the file is over，waiting upload,the initial
#define        UPLOAD_FILE_UPLOADING            1
//when ftp server begin to upload,change state to 1
#define        UPLOAD_FILE_UPLOAD_SUCCESS       2
//when ftp server upload successful,change state to 2
#define        UPLOAD_FILE_UPLOAD_FAILED        3
//when ftp server failed to upload ,change state to 3
#define        UPLOAD_FILE_UPLOAD_INTIME        4
/*
the file is uploadeded in time
the state changed into 3 only when regular check find the pre-state is 2
*/
#define        UPLOAD_FILE_UPLOAD_LATE          5
/*
the file isn't uploadeded in time
the state changed into 4 when regular check find the pre-state is 0 or 1 or 3
*/
#define        UPLOAD_FILE_NONEXIST             6
/*
the file isn't exist
the state changed into 6 when regular check find the node isn't in the list
cteate one and set the state to 6
*/
#define        UPLOAD_FILE_UNKNOWN              7
/*
the file is exist,but didn't upload in time
*/


#ifdef DEBUG
#define CHECKTASK_LOG_PATH "./"
// path of checking log file
#define CHECKTASK_LOG_PATH_SIZE 30
// max size of the path of checking log file
#define CHECKTASK_LOG_TYPE ".txt"
// the type of the checking file
#endif



typedef struct UploadNode
{
    char * filename;
    char * analysisCenterPath;
    char * productCenterPath;
    char state;
    char type;
    FtpServer * server;
    struct UploadNode * next;
}UploadNode,* UploadList;

/*
the strcucture of the upload list
it included filename and state and the point to the next node
filename records the name of the upload file,end with .z
type means the file is GNSS or BEIDOU
state indicates the state of the upload,it include 5 state
UPLOAD_FILE_EXIST(0):           can be uploaded
UPLOAD_FILE_UPLOADING(1):       uploading
UPLOAD_FILE_UPLOAD_SUCCESS(2):  uploaded successful
UPLOAD_FILE_UPLOAD_FAILED(3):   upload failed
UPLOAD_FILE_UPLOAD_INTIME(4):   upload in time
UPLOAD_FILE_UPLOAD_LATE(5):     upload not in time
UPLOAD_FILE_DELETABLE(6):       the node can de deleted
*/

#ifdef DEBUG
char * getchartime();
void log_checktask(char * name,char * a);
void display();
#endif

void up_delay();
struct tm * gettime();
void initUploadlist();
void insertlist(UploadNode * p0);
void  delUpload(UploadNode * up);
void checkfile(char * name,int type);
void  freeUploadNode(UploadNode * up);
int IsLeapYear(int year);
int GetDaysOfMonth(int year,int month,int day);
int BTS_Time(int year,int month,int day);
int fileIsExist(char *filePath);
void copyfile(char * filename,int type,char * dir);
int nodeIsExist(char * name,int type);
static void _inotify_event_handler(struct inotify_event *event);
void analysisCenterMonitor();
void hourtask(int wwww,int d,int hr);
void hour6task(int wwww,int d,int hr);
void daytask(int wwww,int d);
void daytask1(int wwww,int d);
void weektask(int wwww);
void monthtask(int yyyy,int mm);
void monthtask1(int yyyy,int mm);
void analysisCenterCheckTask();

#endif
