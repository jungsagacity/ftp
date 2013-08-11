#ifndef _UTILITY_H_
#define _UTILITY_H_ 1

typedef struct uploadPathInfo
{
    char * BDLocalPathPrefix;
    char * GNSSLocalPathPrefix;
    char * BDLocalPathPrefixBak;
    char * GNSSLocalPathPrefixBak;
    char * BDRemotePathPrefix;
    char * GNSSRemotePathPrefix;

}UploadPath;


int clearSpace(char *str);
void delay();
int writeLog(char *format,...);
int loadSystem(char *conf);
int mergeStationList(char * name, char (*stationFileNames)[MAX_STATION_FILE_NAME_SIZE], int arrayLines);
int stationListReload();
int initList();
void tranverseStationList();
void tranverseFtpServerList();
#endif
