#ifndef _UTILITY_H_
#define _UTILITY_H_

#define MAX_STATION_FILE_NAME_SIZE 1000

#define DEBUG 1

typedef struct uploadPathInfo
{
    char * BDLocalPathPrefix;
    char * GNSSLocalPathPrefix;
    char * BDLocalPathPrefixBak;
    char * GNSSLocalPathPrefixBak;
    char * BDRemotePathPrefix;
    char * GNSSRemotePathPrefix;

}UploadPath;


typedef struct ftpserver
{
    char    *ip;
    int     port;
    char    *username;
    char    *passwd;
    struct  ftpserver * next;
}FtpServer;

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


int clearSpace(char *str);
void delay();
int writeLog(char *format,...);
int loadSystem(char *conf);
int stationListReload();
int initList();
void tranverseStationList();
void tranverseFtpServerList();
int mergeStationList(char * name, char (*stationFileNames)[MAX_STATION_FILE_NAME_SIZE], int arrayLines);


#endif
