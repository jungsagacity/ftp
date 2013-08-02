#ifndef __UPLOAD_H__
#define __UPLOAD_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/inotify.h>
#include <malloc.h>
#include <time.h>

#include "global.h"

/********  MACRO DEFINITION  ********/
#define PRODUCT_CENTER_PATH_PREFIX "upload/"
#define INOTIFY_EVENTS IN_CLOSE_WRITE|IN_DELETE
//events of monitor
#define BUF_MAX 1024
// the max size of buffer to store the temporary monitor events
#define UNIX_Z ".Z"
//the suffix of compression
#define PRE_FILENAME_SIZE 4
//the size of the prefix of filename ,like"ACC","ACI"..
#define POST_FILENAME_SIZE 8
//the size of the suffix of filename ,like".irt.Z"
#define STD_FILENAME_SIZE 20
//the max size of the standard filename ,like"ACUwwwwd_HR.sp3.Z"
#define COMMAND_SIZE 50
//to storre compression command,include full path and filename
#define BEIDOU_YEAR 2006
#define BEIDOU_MONTH 1
#define BEIDOU_DAY 1
//the first day of the BEIDOU week

#define        UPLOAD_FILE_EXIST                0
#define        UPLOAD_FILE_UPLOADING            1
#define        UPLOAD_FILE_UPLOAD_SUCCESS       2
#define        UPLOAD_FILE_UPLOAD_FAILED        3
#define        UPLOAD_FILE_UPLOAD_INTIME        4
#define        UPLOAD_FILE_UPLOAD_LATE          5
#define        UPLOAD_FILE_NONEXIST             6
#define        UPLOAD_FILE_DELETABLE            7
#define        UPLOAD_FILE_UNKNOWN              8


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
    char filename[STD_FILENAME_SIZE];
    int state;
    struct UploadNode * next;
}UploadNode;

/*
the strcucture of the upload list
it included filename and state and the point to the next node
filename records the name of the upload file,end with .z
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

struct tm * gettime();
void insertlist(UploadNode * p0);
void search(char * name);
int IsLeapYear(int year);
int GetDaysOfMonth(int year,int month,int day);
int BTS_Time(int year,int month,int day);
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
