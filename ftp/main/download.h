#ifndef _DOWNLOAD_H_

#include "global.h"

// read the infomation
#define     BUF_SIZE            1024
//#define     DOWN_INFO_PATH      "downloadInfo.txt"//the path of the download information file
//#define     DEBUG_1               1                //readfile_printf


//return the file exist or not
#define	DOWNLOAD_FILE_DEFAULT			1
#define	DOWNLOAD_FILE_NONEXIST			2
#define DOWNLOAD_FILE_EXIST				3
#define	DOWNLOAD_FAILED	                4


#define   MAX_size       1000
#define   R_year         366           //Days of leap year
#define   P_year         365           //Days of leap year
#define   Daily          "daily"
#define   Hourly         "hourly"
#define   Highrate       "highrate"
#define   stationame     "ssss"
#define   yyyy           "/yyyy/"
#define   ddd            "/ddd/"
#define   hh             "/hh/"
#define   mm             "/mm/"
#define   yyf            "/yyf/"
#define   extensioname   ".inmp"
#define   success_extension ".Z"


typedef struct        //自定义时间数据结构
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} MYtime;


/*
  struct of the download information
  序号，数据源，时间类型，文件类型，站点列表文件，
  下载服务器，数据中心文件格式，分析中心存放根目录
*/
typedef  struct DNode
{
    char *filename;      //download file name;
    char *remotePath;    //file address in remote ftp server
    char *localPath;     //where file needed to place
    char (*station)[5]; //the name of a string array storing the staion names.
    char *state ;        //every char corresponds to one station in the next member variable char *stations
    int   taskNum;       //task number.
    FtpServer *server;
    struct DNode *next;
} DownloadNode, *DownloadList;

/*
  struct of the download information
  序号，数据源，时间类型，文件类型，站点列表文件，
  下载服务器，数据中心文件格式，分析中心存放根目录
*/
typedef struct downInfo
{
    int  id;
    char *source;
    char *timeType;
    char *fileType;
    char *stationList;
    char *downloadServer;
    char *dataCenterPath;
    char *localPath;
    struct downInfo * next;
} DownInfo;


//creat download list
void get_currentime(MYtime *mt);
char *replace(char *str1,char *str2,char *str3);
char *replace_path(char *path1,int year,int day,int hour,int minute,char *type);
void creat_filename(int year,int day,int hour,int minute,char *type,char *time_type,char filename[]);
char Search_undownload_file(char filename[]);
int  transfer(int year,int month,int day);
void creat_list(int year,int day,int hour,int minute,DownInfo *DI);
void add_Info(DownloadNode *s,DownInfo *p,int year,int day,int hour,int minute);
void time_module_control();
char Search_file(char *filename);

//read the request file
int readDownloadInfo(char * downloadInfoFile, DownInfo * downInfoList);
void removespace(char  * str);
void  delDownInfo(DownInfo * downInfoList,DownInfo * down);
DownInfo * addDownInfo(DownInfo * downInfoList,DownInfo * down);
DownInfo * initDownInfolist();
void displayDW(DownInfo * head);

#endif
