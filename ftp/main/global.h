/*
*	mainly contains some global variable needed to be initialized accroding to properly runing envirement.
*/



#ifndef _GLOBAL_H_
#define _GLOBAL_H_ 	1


/*-------------PRODUCT CENTER setting---------------------------------------
*
*-------------------------------------------------------------------------*/
#define 	UP_ANALYSIS_CENTER_PATH_PREFIX1		"/home/jung/ftp/upload/iGMAS"
#define 	UP_ANALYSIS_CENTER_PATH_PREFIX2		"/home/jung/ftp/upload/iGIS"
#define		PRODUCT_CENTER_PATH_PREFIX			"upload/"


/*-------------DATA CENTER setting---------------------------------------------
*
*-------------------------------------------------------------------------*/
#define   	DW_ANALYSIS_CENTER_PATH_PREFIX      "/home/jung/tmp/FromDC/"
#define   	DATA_CENTER_PATH_PREFIX           	"download/iGMAS/"
#define 	DATA_SATION							"ssss"

/*-------------LOG setting---------------------------------------------
*
*-------------------------------------------------------------------------*/
#define 	UPLOAD_LOG_FILE			"upload.log"
#define 	DOWNLOAD_LOG_FILE		"download.log"


//-------------FTP setting---------------------------------//
#define DATA_CENTER_FTP_SERVER_IP   		"192.168.0.222"
#define DATA_CENTER_FTP_SERVER_PORT			21
#define DATA_CENTER_FTP_USER_NAME			"public"
#define DATA_CENTER_FTP_USER_PASSWORD		"123456"

#define PRODUCT_CENTER_FTP_SERVER_IP   		"192.168.0.222"
#define PRODUCT_CENTER_FTP_SERVER_PORT		21
#define PRODUCT_CENTER_FTP_USER_NAME		"public"
#define PRODUCT_CENTER_FTP_USER_PASSWORD	"123456"

#define	MAX_CONNECT_TIMES					3
//---------------------------------------------------------//


/*-------------DEBUG setting---------------------------------------
*
*-------------------------------------------------------------------------*/
#define DEBUG							1
#define	DOWNLOAD_TASK_PRINT_NUM			50



typedef struct ftpserver
{
    char    *ip;
    int     port;
    char    *username;
    char    *passwd;
    struct  ftpserver * next;
}FtpServer;

typedef struct stationlist
{
    char *name;
    char (*station)[5];
    struct stationlist * next;
}StationList;

typedef struct uploadPathInfo
{
    char * BDLocalPathPrefix;
    char * GNSSLocalPathPrefix;
    char * BDLocalPathPrefixBak;
    char * GNSSLocalPathPrefixBak;
    char * BDRemotePathPrefix;
    char * GNSSRemotePathPrefix;

}UploadPath;

#endif
