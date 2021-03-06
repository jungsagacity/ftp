#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdarg.h>

#include "ftp.h"
#include "download.h"

/******************************************************************************************
*                                   global variables                                      *
******************************************************************************************/


typedef struct uploadPathInfo
{
    char * BDLocalPathPrefix;
    char * GNSSLocalPathPrefix;
    char * BDLocalPathPrefixBak;
    char * GNSSLocalPathPrefixBak;
    char * BDRemotePathPrefix;
    char * GNSSRemotePathPrefix;

}UploadPath;


char    *tempDownloadFileSuffix;//
char    *tempUploadFileSuffix;
char    *downloadInfoFile;
int     maxDownloadTaskNum;


FtpServer   *fs;//store all ftp server info in system.ini file
UploadPath  *uploadPath;//stor upload path info in sysytem.ini file
DownInfo    *downInfoList;// all download task recorded in the list.
StationList sl;//download station list info in system.ini  file.


extern  DownloadList downloadList;// extern from download.c file
pthread_mutex_t downloadMutex = PTHREAD_MUTEX_INITIALIZER;


/************************************************************************************************
*                                   functions                                                   *
*************************************************************************************************/


/************************************************************************************************
*    function   :   clear all blank space char
*    para       :   {char *str} the target string
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
*************************************************************************************************/
int clearSpace(char *str)
{
    if(str == NULL) return -1;
    if(strlen(str) == 0) return -1;

    char *p = str;
    int i=0;

    while((*p) !=0)
    {
        if((*p) != ' ')
        {
            str[i++] = *p;
        }

        p ++;
    }
    str[i] = 0;

    return 0;
}

/************************************************************************************************
*    function   :   delay some time
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
*************************************************************************************************/
void delay()
{
    int b=0,e=0;
    for(b=0;b<2000;b++)
    for(e=0;e<2000;e++);
}

/*************************************************************************************************
*    function   :   periodically check if there is task needed to be excuted;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/

void timingTask()
{
	// record current time with two kind of time formation and transform mutually
	struct tm 	*st;
	time_t 		t;

	// 5 minute point at which needed to check task.
	int minute[5] = {0,10,25,40,55};
	int tempMin = 0, temphour = 0, tempday = 0;

	//temporary struct taskMinute used to transfer the 5 minute point in case of repetitive excution
	struct taskMinute
	{
		int min[5];//record the check minute of every hour
		int hour;//record the check minute of every hour
		int day;
		char traverseState[5];//record traverse state,0;uncheck,1:checked
	}taskMins;

	// initialise the struct array,making sure the traverseState is zero
	int i = 0;
	for( i=0;i<5;i++)
	{
		(taskMins.min)[i] = minute[i];
		(taskMins.traverseState)[i] = 0;
	}

	t=time(NULL);
    st = localtime(&t);//get current datetime
    tempMin = st->tm_min;//catch the current minute
    taskMins.hour = st->tm_hour;
    taskMins.day = st->tm_mday;


	//main process
	while(1)
	{

		{// every second excute the main process
			t=time(NULL);
			st = localtime(&t);//get current datetime
			tempMin = st->tm_min;//catch the current minute
            temphour = st->tm_hour;
            tempday = st->tm_mday;
            //delay some time and check task again.
            delay();
		}

		//check whether there exist excutable task.
		for( i = 0; i < 5; i ++)
		{
			if( (tempMin == (taskMins.min)[i])  && ( (taskMins.traverseState)[i] == 0)  )
			{
				if( i == 0)
				{
					#ifdef DEBUG
                    printf("analysis center checking task.\n");
					#endif

				}
				else
				{
					#ifdef DEBUG
                    printf("data center checking task.\n");
					#endif
                    pthread_mutex_lock(&downloadMutex);//lock
					time_module_control();
					pthread_mutex_unlock(&downloadMutex);//unlock
				}

				(taskMins.traverseState)[i] = 1;
			}

		}

		if ((temphour != taskMins.hour) || (tempday != taskMins.day))
		{
            taskMins.hour = temphour;
            taskMins.day = tempday;
            for( i=0;i<5;i++)
            {
                (taskMins.traverseState)[i] = 0;
            }
		}
	}
}



/*************************************************************************************************
*    function   :   rename local file.
*    para       :   {char *oldname} old name;
*                   {char *newname} new name.
*
*    return     :   {int } 0: ok; -1:error.
*
*    history    :   {2013.8.5 wujun} frist create;
**************************************************************************************************/
int localRename(char * oldname ,char * newname)
{
    if( (oldname == NULL) || (newname == NULL) )
    {
        #ifdef DEBUG
        printf("Fun(localRename): one of parameter is NULL.\n");
        #endif
        return -1;
    }

    char *command;
    int len = strlen(oldname) + strlen(newname) + 10;
    command = (char *) malloc(sizeof(char) * len);
    if( command == NULL )
    {
        #ifdef DEBUG
        printf("Fun(localRename): malloc \"commmand\" error.\n");
        #endif
        return -1;
    }

    FILE *pf;
    sprintf(command,"mv %s %s",oldname,newname);//get the command to compress the file with unix.z

    if((pf=popen(command,"r"))==NULL)//execute the command of compression
    {
        #ifdef _DEBUG
        printf("Fun(localRename): popen command error.\n");
        #endif
        return -1;
    }
    pclose(pf);

    return 0;
}

/*************************************************************************************************
*    function   :   write log to file
*    para       :   {char *format} content format;
*
*    return     :   {int } 0: ok; -1:error.
*
*    history    :   {2013.8.5 wujun} frist create;
**************************************************************************************************/
int writeLog(char *format,...)
{
   	if( format == NULL)
   	{
        #ifdef DEBUG
        printf("Fun(writeLog): parameter is Null.\n");
        #endif
        return -1;
   	}

   	struct tm 	*st;
	time_t 		t;

    t = time(NULL);
    st = localtime(&t);//get current datetime

    int day = st->tm_mday + 1;
    int month = st->tm_mon + 1;
    int year = st->tm_year + 1990;

    char logFile[100] ;
    memset(logFile, 0, 100);

    sprintf(logFile, "log/%04d%02d%02d.log", year, month, day);
    dirIsExist("log/");
    FILE *dwLogFp;
    if( (dwLogFp=fopen( logFile ,"at") ) == NULL )
    {
        dwLogFp = fopen( logFile ,"wt");
        if( dwLogFp == NULL )
        {
            #ifdef DEBUG
            printf("Fun(writeLog):create file %s error.\n ",logFile);
            #endif
            return -1;
        }
    }

    va_list arg_ptr;
    va_start(arg_ptr, format);
    vfprintf(dwLogFp, format, arg_ptr);
    va_end(arg_ptr);

    fclose(dwLogFp);

}

/**************************************************************************************************
*    function   :   downlaod sigle file from ftp server
*    para       :   {DownloadNode *dwNode} download task node;
*                   {int nodeid} state id
*    return     :   {int} -1 :error; 0: ok.
*
*    history    :   {2013.7.25 wujun} frist create;
*                   {2013.8.10 wujun}   adopt char array but not string, so we need not consider
                                        the time when we free the allocated memory.
**************************************************************************************************/
int singleDownload(DownloadNode *dwNode, int nodeid)
{
	//downlaod starting time and end time
	char    startTime[100] ;
	char    endTime[100];
	time_t  timer;

	char    realFilename[100],tempFilename[100],capitalFileName[100];
	char    tempLocalFullPath[1000];
	char    remoteFullPath[1000];
	char    localFullPath[1000],localDir[1000];
	char    downloadState = 0;

	char    ftpServerIP[100];
	int     port;
	char    username[100];
	char    passwd[100];
    int     sockfd;


    int     len  = 0;

    pthread_mutex_lock(&downloadMutex);
    if( dwNode->server != NULL)
    {
        if( dwNode->server->ip != NULL)
        {
            memset(ftpServerIP, 0, 100);
            strcpy(ftpServerIP, dwNode->server->ip);
        }
        else
        {
            #ifdef DEBUG
            printf("Fun(singleDownload),dwNode->server->ip is NULL.\n");
            #endif
            return -1;
        }

        if( dwNode->server->username != NULL)
        {
            memset(username, 0, 100);
            strcpy(username, dwNode->server->username);
        }
        else
        {
            #ifdef DEBUG
            printf("Fun(singleDownload),dwNode->server->username is NULL.\n");
            #endif
            return -1;
        }

        memset(passwd, 0, 100);
        if(dwNode->server->passwd != NULL)
        {
            memset(passwd, 0, 100);
            strcpy(passwd, dwNode->server->passwd);
        }

        port = dwNode->server->port;
    }
    else
    {
        #ifdef DEBUG
        printf("Fun(singleDownload),dwNode->server is NULL.\n");
        #endif
        return -1;
    }

    if( dwNode->filename == NULL)
    {
        #ifdef DEBUG
        printf("Fun(singleDownload):downloadNode->fileName is NULL.\n");
        #endif
        return -1;
    }
    else
    {
        len = strlen( dwNode->filename);
        if(  len > 4 )// larger than the lenght of "ssss"
        {
            memset(realFilename, 0, 100);
            strcpy(realFilename,dwNode->filename);

            int i = 0;
            for( i = 0;i < 4; i++ )
            {
                if( dwNode->station == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(singleDownload): dwNode->station is NULL.\n");
                    #endif
                    return -1;
                }
                if((dwNode->station)[nodeid] == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(singleDownload): (dwNode->station)[%d] is NULL.\n",nodeid);
                    #endif
                    return -1;
                }

                if( strlen( (dwNode->station)[nodeid] ) < 4 )
                {
                    #ifdef DEBUG
                    printf("Fun(singleDownload): (dwNode->station)[%d] error.\n",nodeid);
                    #endif
                    return -1;
                }
                #ifdef DEBUG
                printf("(dwNode->station)[%d] = %s\n",nodeid,(dwNode->station)[nodeid]);
                #endif

                realFilename[i] = (dwNode->station)[nodeid][i];//replace ssss

            }
            if( strcmp("210.72.144.2", ftpServerIP ) == 0)
            {
                realFilename[len-3] -= 32;
            }

        }
        else
        {
            #ifdef DEBUG
            printf("Fun(singleDownload):downloadNode->fileName is name error.\n");
            #endif
            return -1;
        }
    }

    if( tempDownloadFileSuffix == NULL)
    {
        #ifdef DEBUG
        printf("Fun(singleDownload):tempDownloadFileSuffix is NULL.\n");
        #endif
        return -1;
    }
    else
    {
        memset(tempFilename, 0, 100);
        sprintf(tempFilename, "%s%s",realFilename, tempDownloadFileSuffix);
    }

    if( (dwNode->localPath) == NULL)
    {
        #ifdef DEBUG
        printf("Fun(singleDownload),dwNode->localPath is NULL.\n");
        #endif
        return -1;
    }
    else
    {
        memset(tempLocalFullPath, 0, 1000);
        sprintf(tempLocalFullPath, "%s%s",dwNode->localPath, tempFilename);

        memset(localDir, 0, 1000);
        strcpy(localDir, dwNode->localPath);

        memset(capitalFileName, 0, 100);
        strcpy(capitalFileName, realFilename);

        int ci = 0;
        for(ci = 0; ci < strlen(capitalFileName); ci++)
        {
            if( capitalFileName[ci] >= 'a' && capitalFileName[ci] <= 'z')
            {
               capitalFileName[ci] -= 32;
            }
        }

        memset(localFullPath, 0, 1000);
        sprintf(localFullPath, "%s%s",dwNode->localPath, capitalFileName);
    }

    if( dwNode->remotePath == NULL )
    {
        #ifdef DEBUG
        printf("Fun(singleDownload),dwNode->remotePath is NULL..\n");
        #endif
        return -1;
    }
    else
    {
        memset(remoteFullPath, 0, 1000);
        sprintf(remoteFullPath, "%s%s",dwNode->remotePath, realFilename);
    }

    if( dwNode->state == NULL )
    {
        #ifdef DEBUG
        printf("Fun(singleDownload),dwNode->state is NULL..\n");
        #endif
        return -1;
    }
    else if( strlen(dwNode->state) > nodeid)
    {
        downloadState = (dwNode->state)[nodeid];
    }
    else
    {
        #ifdef DEBUG
        printf("Fun(singleDownload),dwNode->state size error.\n");
        #endif
        return -1;
    }

    pthread_mutex_unlock(&downloadMutex);

    if( downloadState == DOWNLOAD_FILE_NONEXIST)
    {
        //we have three times to try to connect to ftp server ,if failed. Otherwise send network error.
        int conncectTimes = 0;
        while((sockfd = connectFtpServer(ftpServerIP, port, username, passwd) )<= FTP_CONNECT_FAILED_FLAG)
        {
            if( MAX_CONNECT_TIMES <= ++conncectTimes )
            {
                //to recod failed connnection time
                timer =time(NULL);
                memset(startTime, 0, 100);
                strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );
                writeLog("%s\t%s\t%s/%s\t%s\n",startTime, "DOWNLOAD_CONNNET_FAILED", ftpServerIP,remoteFullPath, " ");
                #ifdef DEBUG
                printf("%s\t%s\t%s/%s\t%s\n",startTime, "DOWNLOAD_CONNNET_FAILED", ftpServerIP,remoteFullPath, " ");
                #endif
                return -1;
            }
            //delay some time and connect ftp again.
            delay();
        }

        if( sockfd > FTP_CONNECT_FAILED_FLAG )
        {
            //recod starting-download time
            timer =time(NULL);
            memset(startTime, 0, 100);
            strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );
            printf("localDir = %s\n",localDir);
            if ( dirIsExist(localDir) == -1)
            {
                #ifdef DEBUG
                printf("Fun(singleDownload): dirIsExist is excuted error.\n");
                #endif
                return -1;
            }

            #ifdef DEBUG
            printf("download... : %s\n",remoteFullPath);
            #endif
            int ftperror = ftp_get(remoteFullPath, tempLocalFullPath, sockfd) ;

            //rename the temparory downloaded file.
            if( access(tempLocalFullPath,0) == 0 )
            {
                if (localRename(tempLocalFullPath, localFullPath) == -1)
                {
                    #ifdef DEBUG
                    printf("Fun(singleDownload): localRename is excuted error.\n");
                    #endif
                    return -1;
                }
            }

            //recod end-download time
            timer =time(NULL);
            memset(endTime, 0, 100);
            strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

            //add event log;
            switch(ftperror)
            {
                case DOWNLOAD_CONNNET_FAILED:
                {
                    #ifdef DEBUG
                    printf("connect error.:%s\n",remoteFullPath);
                    #endif
                    writeLog("%s\t%s\t%s/%s\t%s\n",startTime, "DOWNLOAD_CONNNET_FAILED", ftpServerIP,remoteFullPath, endTime);
                    break;
                }
                case DOWNLOAD_LOCAL_FILENAME_NULL:
                case DOWNLOAD_REMOTE_FILENAME_NULL:
                case DOWNLOAD_CREAET_LOCALFILE_ERROR:
                case DOWNLOAD_CONNECT_SOCKET_ERROR:
                case DOWNLOAD_PORT_MODE_ERROR:
                case DOWNLOAD_REMOTE_FILE_NOEXIST:
                {
                    #ifdef DEBUG
                    printf("download failed:%s\n",remoteFullPath);
                    #endif
                    writeLog("%s\t%s\t%s/%s\t%s\n",startTime, "DOWNLOAD_FAILED", ftpServerIP,remoteFullPath, endTime);
                    break;
                }
                case FTP_DOWNLOAD_SUCCESS:
                {
                    #ifdef DEBUG
                    printf("download success:%s\n",remoteFullPath);
                    #endif
                    writeLog("%s\t%s\t%s/%s\t%s\n",startTime, "DOWNLOAD_SUCCESS", ftpServerIP,remoteFullPath, endTime);
                    break;
                }
                default:break;
            }

            close(sockfd);
            delay();
        }
    }

    return 0;
}

/**************************************************************************************************
*    function   :   download files from data center;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/
void download()
{
    int 	i = 0;
	int     taskNum;
	int     stationNum;
	char    *state;

	//downlaod starting time and end time
	char    startTime[100] = {0};
	char    endTime[100] = {0};
	time_t  timer;

	//ftp
	int     sockfd = 0;
	int     ftperror = 0;//ftp error code

    while(1)
    {
        DownloadNode * p,*pLast;
		pthread_mutex_lock(&downloadMutex);//lock
		p = downloadList->next;//temporary variable always points to uploadList head pointer.

        pthread_mutex_unlock(&downloadMutex);//unlock

		//travers daily task arry which max size is smaller than 1000;
        while( p != NULL )
        {
            if( p->state != NULL )
            {
                stationNum = strlen(p->state);

                for( i = 0; i < stationNum; i++)
                {
                    #ifdef DEBUG
                    printf("stationNum = %d,\t i = %d\n", stationNum,i);
                    #endif
                    //if file does not exist, then start up download process.
                    if((p->state)[i] == DOWNLOAD_FILE_NONEXIST)
                    {
                       printf("file not exist: %s%s\n",p->remotePath,p->filename);
                       singleDownload(p, i);
                    }
                }
            }

            pthread_mutex_lock(&downloadMutex);//lock
            pLast = downloadList->next;
            p->isHandled = HANDLE_YES;
            DownloadNode * q = downloadList;
            while( pLast != NULL )
            {
                if( pLast->isHandled == HANDLE_NO )
                {
                    q = pLast;
                    pLast = pLast->next;
                }
                else if( pLast->isHandled == HANDLE_YES )
                {
                    break;
                }
            }

            q->next = p->next;
            free(p->filename);
            free(p->localPath);
            free(p->remotePath);
            free(p->state);
            free(p);

            p = q->next ;
            pthread_mutex_unlock(&downloadMutex);//unlock
        }

        delay();
    }
}

/**************************************************************************************************
*    function   :   load system.ini file
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int loadSystem(char *conf)
{
    int k=0;
    FILE *fp = fopen(conf, "r");
    if(fp == NULL)
    {
        #ifdef DEBUG
        printf("Fun(loadSystem):%s open error.\n",conf);
        #endif
        return -1;
    }

    FtpServer  *fsNode;

    uploadPath = (UploadPath*)malloc(sizeof(UploadPath));
    memset(uploadPath, 0, sizeof(UploadPath));

    char buff[1000];
    memset(buff, 0, 1000);
    while(fgets(buff, 1000, fp) != NULL)
    {
        if((buff[0] == '#') || (buff[0] == '\n') )
        {
            continue;
        }

        clearSpace(buff);

        int i, j;
        i = j =0;
        while(buff[i++] != ':');
        char *name;
        name = (char *)malloc(i);
        if(name == NULL)
        {
            #ifdef DEBUG
            printf("Fun(loadSystem): malloc \"name\" error.\n");
            #endif
            return -1;
        }

        memset(name, 0, i);
        strncpy(name, buff, i-1);
        j = i;

        int parNum = 0;
        if( !strcmp(name,"productCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
            if(fsNode == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc product center \"fsNode\" error.\n");
                #endif
                return -1;
            }
            memset(fsNode, 0, sizeof(FtpServer));

            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                switch( ++parNum )
                {
                    case 1://ftp server ip
                    {
                        fsNode->ip = (char *)malloc(i-j);
                        if( fsNode->ip == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->ip\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->ip, 0, i-j);
                        strncpy(fsNode->ip,buff+j, i-j-1);
                        j = i;
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
                        j = i;
                        break;
                    }
                    case 3://username
                    {
                        fsNode->username = (char *)malloc(i-j);
                        if( fsNode->username == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->username\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->username, 0, i-j);
                        strncpy(fsNode->username,buff+j, i-j-1);
                        j = i;

                        if( buff[i-1] == '\n')
                        {
                            fsNode->passwd = NULL;
                        }
                        break;
                    }
                    case 4://passwd
                    {
                        fsNode->passwd = (char *)malloc(i-j);
                        if( fsNode->passwd == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->passwd\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->passwd, 0, i-j);
                        strncpy(fsNode->passwd,buff+j, i-j-1);
                        break;
                    }
                    default:break;
                }
            }

            //make sure the product cener fpt info placed in the first node.
            fs->next = fsNode;
            fsNode ->next = NULL;
            fsNode = NULL;
        }

        if( !strcmp(name,"dataCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
            if( fsNode == NULL )
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc dataCenter \"fsNode\" error.\n");
                #endif
                return -1;
            }

            memset(fsNode, 0, sizeof(FtpServer));
            parNum = 0;
            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                switch( ++parNum )
                {
                    case 1://ftp server ip
                    {
                        fsNode->ip = (char *)malloc(i-j);
                        if( fsNode->ip == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->ip\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->ip, 0, i-j);
                        strncpy(fsNode->ip,buff+j, i-j-1);
                        j = i;
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
                        j = i;
                        break;
                    }
                    case 3://username
                    {
                        fsNode->username = (char *)malloc(i-j);
                        if( fsNode->username == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->username\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->username, 0, i-j);
                        strncpy(fsNode->username,buff+j, i-j-1);
                        j = i;

                        if( buff[i-1] == '\n')
                        {
                            fsNode->passwd = NULL;
                        }
                        break;
                    }
                    case 4://passwd
                    {
                        fsNode->passwd = (char *)malloc(i-j);
                        if( fsNode->passwd == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->passwd\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->passwd, 0, i-j);
                        strncpy(fsNode->passwd,buff+j, i-j-1);

                        break;
                    }
                    default:break;
                }
            }

            //make sure the product cener fpt info placed in the first node.
            if(fs->next == NULL)
            {
                fsNode ->next = NULL;
                fs->next = fsNode;

            }else
            {
                FtpServer  *tmp = fs->next->next;
                fsNode ->next = tmp;
                fs->next->next = fsNode;
            }

            fsNode = NULL;
        }

        if( !strcmp(name,"stations"))
        {
            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                char *statesionFileName;
                statesionFileName = (char *)malloc(i-j);
                if( statesionFileName == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"statesionFileName\" error.\n");
                    #endif
                    return -1;
                }
                memset(statesionFileName, 0, i-j);
                strncpy(statesionFileName,buff+j, i-j-1);

                j = i;

                StationNode *slNode,*temp;
                slNode = (StationNode *)malloc(sizeof(StationNode));
                if( slNode == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"slNode\" error.\n");
                    #endif
                    return -1;
                }
                memset(slNode, 0, sizeof(StationNode));

                FILE *fp = fopen(statesionFileName,"r");
                if(fp == NULL)
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): %s does not exist.\n",statesionFileName);
                    #endif
                    return -1;
                }

                char buf[7];
                char (*station)[5];

                while(fgets(buf,7,fp))
                {
                    k++;
                }

                station = (char **)malloc(k*5);
                if(station == NULL)
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"station\" error.\n");
                    #endif
                    return -1;
                }
                memset(station,0,k*5);

                fseek(fp, 0,SEEK_SET);
                int t = 0;
                 while(fgets(buf,7,fp))
                {
                    if(t<k)
                    {
                        strncpy(station[t++], buf, 4);
                    }
                    memset(buf, 0, 7);

                }

                slNode ->name = statesionFileName;
                slNode ->station = station;
                slNode ->stationNum = k;
                temp = sl->next;
                sl->next = slNode;
                slNode ->next = temp;

                fclose(fp);
                delay();
                k = 0;
            }
        }

        if( !strcmp(name,"tempDownloadFileSuffix"))
        {
            while(buff[i++] !='\n');
            tempDownloadFileSuffix = (char *)malloc(i-j);
            if(tempDownloadFileSuffix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"tempDownloadFileSuffix\" error.\n");
                #endif
                return -1;
            }
            memset(tempDownloadFileSuffix, 0, i-j);
            strncpy( tempDownloadFileSuffix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"tempUploadFileSuffix"))
        {
            while(buff[i++] !='\n');
            tempUploadFileSuffix = (char *)malloc(i-j);
            if(tempUploadFileSuffix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"tempUploadFileSuffix\" error.\n");
                #endif
                return -1;
            }
            memset(tempUploadFileSuffix, 0, i-j);
            strncpy( tempUploadFileSuffix, buff+j, i-j-1);
            j = i;
        }

        if( !strcmp(name,"maxDownloadTaskNum"))
        {
            while(buff[i++] !='\n');
            char *num = (char *)malloc(i-j);
            if(num == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"num\" error.\n");
                #endif
                return -1;
            }
            memset(num, 0, i-j);
            strncpy( num, buff+j, i-j-1);
            j = i;
            maxDownloadTaskNum = atoi(num);

        }

        if( !strcmp(name,(const char *)"BDLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefix = (char *)malloc(i-j);
            if(uploadPath->BDLocalPathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDLocalPathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefix, buff+j, i-j-1);
            j = i;
        }

        if( !strcmp(name,"GNSSLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefix = (char *)malloc(i-j);
            if(uploadPath->GNSSLocalPathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSLocalPathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"BDLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefixBak = (char *)malloc(i-j);
            if(uploadPath->BDLocalPathPrefixBak == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDLocalPathPrefixBak\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefixBak, buff+j, i-j-1);
            j = i;

        }


        if( !strcmp(name,"GNSSLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefixBak = (char *)malloc(i-j);
            if(uploadPath->GNSSLocalPathPrefixBak == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSLocalPathPrefixBak\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefixBak, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"BDRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDRemotePathPrefix = (char *)malloc(i-j);
            if(uploadPath->BDRemotePathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDRemotePathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->BDRemotePathPrefix, buff+j, i-j-1);
            j = i;

        }


        if( !strcmp(name,"GNSSRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSRemotePathPrefix = (char *)malloc(i-j);
            if(uploadPath->GNSSRemotePathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSRemotePathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSRemotePathPrefix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"downloadInfoFile"))
        {

            while(buff[i++] !='\n');
            downloadInfoFile = (char *)malloc(i-j);
            if(downloadInfoFile == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"downloadInfoFile\" error.\n");
                #endif
                return -1;
            }
            memset(downloadInfoFile, 0, i-j);
            strncpy( downloadInfoFile, buff+j, i-j-1);
            j = i;

        }
    }

    fclose(fp);
    delay();
}

/**************************************************************************************************
*    function   :   merge staion lists
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int mergeStationList(char * name, char (*stationFileNames)[MAX_STATION_FILE_NAME_SIZE], int arrayLines)
{
    if( (name == NULL)  || (stationFileNames == NULL))
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): parameter is NULL.\n ");
        #endif
        return -1;
    }

    int i;
    int j = 0;
    int totalStations = 0;
    StationNode * p = sl->next;

    for(i=0;i<arrayLines;i++)
    {
        p = sl->next;
        while( p != NULL)
        {
            if( (p->name == NULL) || (stationFileNames[i] == NULL))
            {
                #ifdef DEBUG
                printf("Fun(mergeStationList): p->name or stationFileNames[i] is NULL.\n ");
                #endif
                return -1;
            }

            if(!strcmp(p->name, stationFileNames[i]))
            {
               totalStations += p->stationNum;
            }

            p = p->next;
        }
    }

    char (*newStaionArray)[5];
    newStaionArray = (char **)malloc(totalStations*5);
    if( newStaionArray == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList):malloc \"newStaionArray\" error.\n ");
        #endif
        return -1;
    }

    memset(newStaionArray, 0, totalStations*5);
    totalStations = 0;
    for(i=0;i<arrayLines;i++)
    {
        p = sl->next;
        while( p != NULL)
        {
            if(!strcmp(p->name, stationFileNames[i]))
            {
                int j;
                for(j=0; j< p->stationNum; j++)
                {
                    if( (p->station)[j] == NULL )
                    {
                        #ifdef DEBUG
                        printf("Fun(mergeStationList): (p->station)[j] is NULL.\n ");
                        #endif
                        return -1;
                    }
                    strcpy(newStaionArray[totalStations+j],(p->station)[j]);
                }
                totalStations += p->stationNum;
                break;
            }

            p = p->next;
        }
    }

    int deleteNum = 0;
    for(i=0;i<totalStations-1;i++)
    {
        for(j=i+1;j<totalStations;j++)
        {
            if(!strcmp(newStaionArray[i], newStaionArray[j]) )
            {
                memset(newStaionArray[i], 0, 5);
                deleteNum++;
            }
        }
    }

    StationNode * newStationNode = (StationNode *)malloc(sizeof(StationNode));
    if( newStationNode == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"newStationNode\" error.\n ");
        #endif
        return -1;
    }
    memset(newStationNode, 0, sizeof(StationNode));

    char (*mergeStationArray)[5] = (char **)malloc((totalStations - deleteNum)*5);
    if( mergeStationArray == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"mergeStationArray\" error.\n ");
        #endif
        return -1;
    }
    memset(mergeStationArray, 0, (totalStations - deleteNum)*5);

    j=0;
    for(i=0;i<totalStations;i++)
    {
        if(strlen(newStaionArray[i]))
        {
            strcpy(mergeStationArray[j++], newStaionArray[i]);
        }

    }

    newStationNode->name = (char *)malloc(strlen(name)+1);
    if( newStationNode->name == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"newStationNode->name\" error.\n ");
        #endif
        return -1;
    }
    memset(newStationNode->name, 0 ,strlen(name)+1);
    strcpy(newStationNode->name,name);

    newStationNode->station = mergeStationArray;
    newStationNode ->stationNum = j;
    p = sl->next;
    sl->next = newStationNode;
    newStationNode->next = p;

}

/**************************************************************************************************
*    function   :   reload station list after merging stations.
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int stationListReload()
{
    DownInfo *p = downInfoList->next;
    StationNode *psl;
    int i=0,j=0;


    while( p != NULL )
    {

        i=0; j=0;
        psl = sl->next;

        while(psl != NULL)
        {
            if( ( psl->name == NULL ) || (p->stationList == NULL) )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): psl->name or p->stationList is NULL.\n");
                #endif
                return -1;
            }
            if(!strcmp(p->stationList, psl->name))// not equal to
            {
                break;
            }

            psl  = psl->next;
        }


        if( psl == NULL)
        {
            //get file number according to the character ','
            while( (p->stationList)[i] )
            {
                if((p->stationList)[i] == ',')
                {
                    j++;
                }
                i++;
            }

            //allocate memory
            char (*stationFileName)[MAX_STATION_FILE_NAME_SIZE] = (char **)malloc((j+1)*MAX_STATION_FILE_NAME_SIZE);
            if( stationFileName == NULL )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): malloc \"stationFileName\" error..\n");
                #endif
                return -1;
            }
            memset(stationFileName, 0, (j+1)*MAX_STATION_FILE_NAME_SIZE);

            char *name = malloc(strlen(p->stationList)+1);
            if( stationFileName == NULL )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): malloc \"name\" error..\n");
                #endif
                return -1;
            }
            memset(name, 0, strlen(p->stationList)+1);
            strcpy(name, p->stationList);

            strcpy(stationFileName[0], strtok(p->stationList,","));

            for(i = 1; i <= j; i++)
            {
                strcpy(stationFileName[i], strtok(NULL,","));
            }

            mergeStationList(name, stationFileName, j+1);
            free(stationFileName);
            free(name);

        }

        p = p->next;
    }
}

/**************************************************************************************************
*    function   :   reload station list after merging stations.
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int initList()
{
    downInfoList = (DownInfo*) malloc(sizeof(DownInfo));
    if( downInfoList == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"downInfoList\" error..\n");
        #endif
        return -1;
    }
    memset(downInfoList, 0, sizeof(DownInfo));
    downInfoList ->next = NULL;

    fs=(FtpServer*)malloc(sizeof(FtpServer));
    if( fs == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"fs\" error..\n");
        #endif
        return -1;
    }
    memset(fs, 0, sizeof(FtpServer));
    fs->next = NULL;

    sl = (StationNode *)malloc(sizeof(StationNode));
    if( sl == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"sl\" error..\n");
        #endif
        return -1;
    }
    memset(sl,0,sizeof(StationNode));
    sl->next = NULL;

    downloadList=(DownloadNode*)malloc(sizeof(DownloadNode));
    if( downloadList == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"downloadList\" error..\n");
        #endif
        return -1;
    }
    memset(downloadList, 0, sizeof(DownloadNode));
    downloadList->next = NULL;

}

/**************************************************************************************************
*    function   :   tranverse Station List
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
void tranverseStationList()
{
    StationNode *p = sl->next;

    while(p != NULL)
    {
        int i = p->stationNum;

        printf("i : %d,\tname : %s\n",i,p->name);
        while(i--)
        {
            //printf("%d\t%s\n",i,(p->station)[i]);
        }

        p = p->next;
    }
}

/**************************************************************************************************
*    function   :   tranverse ftp server list
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
void tranverseFtpServerList()
{
    FtpServer *p = fs->next;

    while(p != NULL)
    {
        //printf("%s\t%d\t%s\t%s\t\n",p->ip,p->port,p->username,p->passwd);
        p = p->next;
    }
}

/**************************************************************************************************
*                                       main                                                      *
**************************************************************************************************/

int main()
{
	// start up download function,transfer data from  analysis center to products&service center
    pthread_t downloadTread;
	// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.
	pthread_t timingTaskTread;

    initList();
    loadSystem("conf/system.ini");

    readDownloadInfo(downloadInfoFile, downInfoList);
    stationListReload();
    tranverseStationList();
    tranverseFtpServerList();

    //create the thread upload
    pthread_create(&downloadTread,NULL,download,(void *)NULL);

    //create the thread timing task
    pthread_create(&timingTaskTread,NULL,timingTask,(void *)NULL);

    while(1);
}
