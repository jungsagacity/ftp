#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>


#include "ftp.h"
#include "upload.h"
#include "global.h"
#include "download.h"
#include "log.h"
#include "msem.h"



/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern struct UploadNode * uploadList;//upload.c, file list periodically created by product center.
extern struct UploadNode * tail;//upload.c, file list periodically created by product center.
extern DownloadList downloadList;
extern void analysisCenterMonitor();//upload.c, monitor the temporary dir in product center
void analysisCenterCheckTask();//upload.c, check the prodcut files whether they have been uploaded
extern EventLog *elog;//record event,when an exception Occurs, like ftp connection,abnormal upload or download.
extern pthread_mutex_t uploadMutex;

extern int giSemLog;//log semaphore
extern int giSemUpload;//upload semaphore
extern int giSemDownload;//download semaphore


/*-------------------------------------config.ini-------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/
int dataCenterFtpServerPort;//ftp server port
char dataCenterFtpServerIP[100];//ftp server ip4 or ip6
char dataCenterFtpUserName[100];//fpt server user name
char dataCenterFtpPassword[100];//ftp password belong to user

int productCenterFtpServerPort;//ftp server port
char productCenterFtpServerIP[100];//ftp server ip4 or ip6
char productCenterFtpUserName[100];//fpt server user name
char productCenterFtpPassword[100];//ftp password belong to user



char *tempDownloadFileSuffix;
char *tempUploadFileSuffix;
FtpServer *fs;
StationList *sl;
UploadPath *uploadPath;








/*-------------------------------------FUNCTION-------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/

/*****************************************************************************************
*    function   :   clear all blank space char
*    para       :   {char *str} the target string
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
******************************************************************************************/
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


/**************************************************************************************************
*    function   :   delay some time
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/

void delay()
{
    int b=0,e=0;
    for(b=0;b<2000;b++)
    for(e=0;e<2000;e++);
}


/************************************************************************************************
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
	int tempMin = 0;

	//temporary struct taskMinute used to transfer the 5 minute point in case of repetitive excution
	struct taskMinute
	{
		int min;//record the check minute of every hour
		char traverseState;//record traverse state,0;uncheck,1:checked
	}taskMins[5];

	// initialise the struct array,making sure the traverseState is zero
	int i = 0;
	for( i=0;i<5;i++)
	{
		taskMins[i].min = minute[i];
		taskMins[i].traverseState = 0;
	}

	//main process
	while(1)
	{

		{// every second excute the main process
			t=time(NULL);
			st = localtime(&t);//get current datetime
			tempMin = st->tm_min;//catch the current minute

            //delay some time and check task again.
            delay();
		}

		//check whether there exist excutable task.
		for( i = 0; i < 5; i ++)
		{
			if(tempMin == taskMins[i].min && taskMins[i].traverseState == 0)
			{
				if( i == 0)
				{
					#ifdef DEBUG
						printf("analysis center checking task.\n");
					#endif
					//analysisCenterCheckTask();
				}else
				{
					#ifdef DEBUG
						printf("data center checking task.\n");
					#endif

                    //pthread_mutex_lock(&downloadMutex);//lock
					//module_control();
					//pthread_mutex_unlock(&downloadMutex);//unlock
				}

				taskMins[i].traverseState = 1;
			}

		}

		/* initialise the traverse state when minute equals to 56,that is to say,
		* when the last minute point is traversed.
		*/
        if( (taskMins[4].traverseState == 1) && (tempMin == (taskMins[4].min + 1) ) )
		{
			//initialise the struct array;
			for( i=0;i<5;i++)
			{
				taskMins[i].traverseState = 0;
			}
		}

	}


}








int config(char *conf)
{
    int k=0;
    FILE *fp;
    fp = fopen(conf, "r");

    char buff[1000];
    memset(buff, 0, 1000);


    fs=(FtpServer*)malloc(sizeof(FtpServer));
    memset(fs, 0, sizeof(FtpServer));
    fs->next = NULL;

    sl = (StationList *)malloc(sizeof(StationList));
    memset(sl,0,sizeof(StationList));
    sl->next = NULL;

    uploadPath = (UploadPath*)malloc(sizeof(UploadPath));
    memset(uploadPath, 0, sizeof(UploadPath));


    if(fp == NULL)
    {
        printf("open file error.\n");
        return -1;
    }else
    {
        while(fgets(buff, 1000, fp) != NULL)
        {
            if((buff[0] == '#') || (buff[0] == '\n') )
            {
                continue;
            }

            int i, j;
            i = j =0;
            while(buff[i++] != ':');
            char *name;
            name = (char *)malloc(i);
            memset(name, 0, i);
            strncpy(name, buff, i-1);
            j = i;
            clearSpace(name);

            int parNum = 0;

            if( !strcmp(name,"productCenterFtp"))
            {
                FtpServer * fsNode = (FtpServer*)malloc(sizeof(FtpServer));
                memset(fsNode, 0, sizeof(FtpServer));

                while(buff[i-1] !='\n')
                {
                    while(buff[i++] != '\t' && buff[i-1] != '\n');
                    switch( ++parNum )
                    {
                        case 1://ftp server ip
                        {
                            fsNode->ip = (char *)malloc(i-j);
                            memset(fsNode->ip, 0, i-j);
                            strncpy(fsNode->ip,buff+j, i-j-1);
                            j = i;
                            clearSpace(fsNode->ip);
                            #ifdef DEBUG
                            printf("ip:%s\n",fsNode->ip);
                            #endif
                            break;
                        }
                        case 2://port
                        {
                            char port[10] = {0};
                            strncpy(port,buff+j, i-j-1);
                            fsNode->ip = atoi(port);
                            j = i;
                            #ifdef DEBUG
                            printf("port:%d\n",fsNode->ip);
                            #endif
                            break;
                        }
                        case 3://username
                        {
                            fsNode->username = (char *)malloc(i-j);
                            memset(fsNode->username, 0, i-j);
                            strncpy(fsNode->username,buff+j, i-j-1);
                            j = i;
                            clearSpace(fsNode->username);

                            if( buff[i-1] == '\n')
                            {
                                fsNode->passwd = NULL;
                            }

                            #ifdef DEBUG
                            printf("username:%s\n",fsNode->username);
                            #endif
                            break;
                        }
                        case 4://passwd
                        {
                            fsNode->passwd = (char *)malloc(i-j);
                            memset(fsNode->passwd, 0, i-j);
                            strncpy(fsNode->passwd,buff+j, i-j-1);
                            clearSpace(fsNode->passwd);
                            #ifdef DEBUG
                            printf("passwd:%s\n",fsNode->passwd);
                            #endif
                            break;

                        }
                        default:break;
                    }

                }

                //make sure the product cener fpt info placed in the first node.
                fs->next = fsNode;
                fsNode ->next = NULL;

            }

            if( !strcmp(name,"dataCenterFtp"))
            {
                FtpServer * fsNode = (FtpServer*)malloc(sizeof(FtpServer));
                memset(fsNode, 0, sizeof(FtpServer));

                while(buff[i-1] !='\n')
                {
                    while(buff[i++] != '\t' && buff[i-1] != '\n');
                    switch( ++parNum )
                    {
                        case 1://ftp server ip
                        {
                            fsNode->ip = (char *)malloc(i-j);
                            memset(fsNode->ip, 0, i-j);
                            strncpy(fsNode->ip,buff+j, i-j-1);
                            j = i;
                            clearSpace(fsNode->ip);
                            #ifdef DEBUG
                            printf("ip:%s\n",fsNode->ip);
                            #endif
                            break;
                        }
                        case 2://port
                        {
                            char port[10] = {0};
                            strncpy(port,buff+j, i-j-1);
                            fsNode->ip = atoi(port);
                            j = i;
                            #ifdef DEBUG
                            printf("port:%d\n",fsNode->ip);
                            #endif
                            break;
                        }
                        case 3://username
                        {
                            fsNode->username = (char *)malloc(i-j);
                            memset(fsNode->username, 0, i-j);
                            strncpy(fsNode->username,buff+j, i-j-1);
                            j = i;
                            clearSpace(fsNode->username);

                            if( buff[i-1] == '\n')
                            {
                                fsNode->passwd = NULL;
                            }

                            #ifdef DEBUG
                            printf("username:%s\n",fsNode->username);
                            #endif
                            break;
                        }
                        case 4://passwd
                        {
                            fsNode->passwd = (char *)malloc(i-j);
                            memset(fsNode->passwd, 0, i-j);
                            strncpy(fsNode->passwd,buff+j, i-j-1);
                            clearSpace(fsNode->passwd);
                            #ifdef DEBUG
                            printf("passwd:%s\n",fsNode->passwd);
                            #endif
                            break;

                        }
                        default:break;
                    }

                }

                //make sure the product cener fpt info placed in the first node.
                if(fs->next == NULL)
                {
                    fs->next = fsNode;
                    fsNode ->next = NULL;
                }else
                {
                    fs->next->next = fsNode;
                    fsNode ->next = NULL;
                }

            }

            if( !strcmp(name,"stations"))
            {
                while(buff[i] !='\n')
                {
                    while(buff[++i] != '\t' && buff[i] != '\n');
                    char *statesionFileName;
                    statesionFileName = (char *)malloc(i-j);
                    memset(statesionFileName, 0, i-j);
                    strncpy(statesionFileName,buff+j, i-j);
                    clearSpace(statesionFileName);
                    j = i+1;
                    printf("statesionFileName:%s\n",statesionFileName);

                    StationList *slNode;
                    slNode = (StationList *)malloc(sizeof(StationList));
                    memset(slNode, 0, sizeof(StationList));

                    FILE *fp = fopen(statesionFileName,"r");
                    if(fp == NULL)
                    {
                        printf("The file %s does not exist.\n",statesionFileName);
                        return -1;
                    }

                    char buf[7];
                    char (*station)[5];

                    while(fgets(buf,7,fp))
                    {
                        k++;
                    }

                    printf("k = %d\n",k);

                    station = (char **)malloc(k*5);
                    memset(station,0,k*5);

                    fseek(fp, 0,SEEK_SET);
                     while(fgets(buf,7,fp))
                    {
                        strncpy(station[--k], buf, 4);
                        //printf("%s\n",station[k]);
                    }

                    slNode ->name = statesionFileName;
                    slNode ->station = station;
                    sl->next = slNode;
                    slNode ->next = NULL;

                    free(station);
                    fclose(fp);
                    k = 0;
                }
            }

            if( !strcmp(name,"tempDownloadFileSuffix"))
            {
                while(buff[i++] !='\n');
                tempDownloadFileSuffix = (char *)malloc(i-j);
                memset(tempDownloadFileSuffix, 0, i-j);
                strncpy( tempDownloadFileSuffix, buff+j, i-j-1);
                clearSpace(tempDownloadFileSuffix);
                j = i;
                printf("%s\n",tempDownloadFileSuffix);
            }

            if( !strcmp(name,"tempUploadFileSuffix"))
            {
                while(buff[i++] !='\n');
                tempUploadFileSuffix = (char *)malloc(i-j);
                memset(tempUploadFileSuffix, 0, i-j);
                strncpy( tempUploadFileSuffix, buff+j, i-j-1);
                clearSpace(tempUploadFileSuffix);
                j = i;
                printf("%s\n",tempUploadFileSuffix);
            }

            if( !strcmp(name,"BDLocalPathPrefix"))
            {
                while(buff[i++] !='\n');
                uploadPath->BDLocalPathPrefix = (char *)malloc(i-j);
                memset(uploadPath->BDLocalPathPrefix, 0, i-j);
                strncpy( uploadPath->BDLocalPathPrefix, buff+j, i-j-1);
                clearSpace(uploadPath->BDLocalPathPrefix);
                j = i;
                printf("%s\n",uploadPath->BDLocalPathPrefix);

            }

            if( !strcmp(name,"GNSSLocalPathPrefix"))
            {
                while(buff[i++] !='\n');
                uploadPath->GNSSLocalPathPrefix = (char *)malloc(i-j);
                memset(uploadPath->GNSSLocalPathPrefix, 0, i-j);
                strncpy( uploadPath->GNSSLocalPathPrefix, buff+j, i-j-1);
                clearSpace(uploadPath->GNSSLocalPathPrefix);
                j = i;
                printf("%s\n",uploadPath->GNSSLocalPathPrefix);

            }

            if( !strcmp(name,"BDLocalPathPrefixBak"))
            {
                while(buff[i++] !='\n');
                uploadPath->BDLocalPathPrefixBak = (char *)malloc(i-j);
                memset(uploadPath->BDLocalPathPrefixBak, 0, i-j);
                strncpy( uploadPath->BDLocalPathPrefixBak, buff+j, i-j-1);
                clearSpace(uploadPath->BDLocalPathPrefixBak);
                j = i;
                printf("%s\n",uploadPath->BDLocalPathPrefixBak);
            }


            if( !strcmp(name,"GNSSLocalPathPrefixBak"))
            {
                while(buff[i++] !='\n');
                uploadPath->GNSSLocalPathPrefixBak = (char *)malloc(i-j);
                memset(uploadPath->GNSSLocalPathPrefixBak, 0, i-j);
                strncpy( uploadPath->GNSSLocalPathPrefixBak, buff+j, i-j-1);
                clearSpace(uploadPath->GNSSLocalPathPrefixBak);
                j = i;
                printf("%s\n",uploadPath->GNSSLocalPathPrefixBak);

            }

            if( !strcmp(name,"BDRemotePathPrefix"))
            {
                while(buff[i++] !='\n');
                uploadPath->BDRemotePathPrefix = (char *)malloc(i-j);
                memset(uploadPath->BDRemotePathPrefix, 0, i-j);
                strncpy( uploadPath->BDRemotePathPrefix, buff+j, i-j-1);
                clearSpace(uploadPath->BDRemotePathPrefix);
                j = i;
                printf("%s\n",uploadPath->BDRemotePathPrefix);

            }


            if( !strcmp(name,"GNSSRemotePathPrefix"))
            {
                while(buff[i++] !='\n');
                uploadPath->GNSSRemotePathPrefix = (char *)malloc(i-j);
                memset(uploadPath->GNSSRemotePathPrefix, 0, i-j);
                strncpy( uploadPath->GNSSRemotePathPrefix, buff+j, i-j-1);
                clearSpace(uploadPath->GNSSRemotePathPrefix);
                j = i;
                printf("%s\n",uploadPath->GNSSRemotePathPrefix);

            }

        }
    }

    fclose(fp);
}

int main()
{
    config("system.ini");

}

/*
int main()
{

	// monitor analysis center,update the gloable variable uploadList, is always running
	pid_t analysisCenterMonitor;
	// start up uoload function,transfer data from  analysis center to products&service center
    pid_t upload;
	// start up download function,transfer data from  data center to analysis center
	pid_t download;
	// log file uploading or downloading action state whether success or failed
	pid_t log;
	// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.
	pid_t timingTask;


	//initilise relative FTP parameters,like server ip, port,user name,password and so on.
	initFTP();
	//initilise upload list
    initUploadlist();
    //initilise upload list
    initDownloadloadlist();
    //initialise sem_upload,sem_download state.
    initSem();

	//create five processes,but never destroy them.
	{
        //create t analysis Center   Monitor process
        analysisCenterMonitor = fork();
        if( analysisCenterMonitor == 0)
        {
            analysisCenterMonitor();
            return 0;
        }

        //create the upload process
        upload = fork();
        if( upload == 0)
        {
            upload();
            return 0;
        }

        //create the download process
        download = fork();
        if( download == 0)
        {
            download();
            return 0;
        }

        //create the timingTask process
        timingTask = fork();
        if( timingTask == 0)
        {
            timingTask();
            return 0;
        }

        //create the log process
        log = fork();
        if( log == 0)
        {
            log();
            return 0;
        }
	}


}
*/










/**************************************************************************************************
*    function   :   downlaod sigle file from ftp server
*    para       :   {DownloadNode *dwNode} download task node;
*                   {int nodeid} state id
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/
void sigleDownload(DownloadNode *dwNode, int nodeid)
{
	//downlaod starting time and end time
	char    startTime[100] = {0};
	char    endTime[100] = {0};
	time_t  timer;

	char    *realFilename,*tempFilename;
	char    *tempLocalFullPath;
	char    *remoteFullPath;
	char    *localFullPath;
	char    downloadState = 0;

	char    *ftpServerIP;
	int     port;
	char    *username;
	char    *passwd;
    int     sockfd;
    sem_p(giSemDownload);

	int len = strlen(dwNode->filename) + 1;
	realFilename = (char *)malloc(len);
	memset(realFilename, 0, len);
	strcpy(realFilename,dwNode->filename);

	int i = 0;
	for(i=0;i<4;i++)
	{
        realFilename[i] = dwNode->stations[nodeid][i];//replace ssss
	}

    len += strlen(tempDownloadFileSuffix) + 1;
	tempFilename = (char *)malloc(len);
	memset(tempFilename, 0, len);
	sprintf(tempFilename, "%s%s",realFilename, tempDownloadFileSuffix);

	len = strlen(dwNode->localPath) + strlen(tempFilename) + 1;
	tempLocalFullPath = (char *)malloc(len);
	memset(tempLocalFullPath, 0, len);
	sprintf(tempLocalFullPath, "%s%s",dwNode->localPath, tempFilename);

	len = strlen(dwNode->localPath) + strlen(realFilename) + 1;
	localFullPath = (char *)malloc(len);
	memset(localFullPath, 0, len);
	sprintf(localFullPath, "%s%s",dwNode->localPath, realFilename);

	len = strlen(dwNode->remotePath) + strlen(realFilename) + 1;
	remoteFullPath = (char *)malloc(len);
	memset(remoteFullPath, 0, len);
	sprintf(remoteFullPath, "%s%s",dwNode->remotePath, realFilename);

	downloadState = dwNode->state[nodeid];

    len = strlen(dwNode->server->ip) + 1;
    ftpServerIP = (char *)malloc(len);
    memset(ftpServerIP, 0, len);
    strcpy(ftpServerIP, dwNode->server->ip);

    len = strlen(dwNode->server->username) + 1;
    username = (char *)malloc(len);
    memset(username, 0, len);
    strcpy(username, dwNode->server->username);

    len = strlen(dwNode->server->passwd) + 1;
    passwd = (char *)malloc(len);
    memset(passwd, 0, len);
    strcpy(passwd, dwNode->server->passwd);

    port = dwNode->server->port;

    sem_v(giSemDownload);

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

                //add event log;
                sem_p(giSemLog);//lock
                addEventLog(DOWNLOAD_CONNNET_FAILED, remoteFullPath, startTime,"");
                sem_v(giSemLog);//unlock

                break;
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

            int ftperror = ftp_get(remoteFullPath, tempLocalFullPath, sockfd) ;
            localRename(tempLocalFullPath, localFullPath);//..........................
            //recod end-download time
            timer =time(NULL);
            memset(endTime, 0, 100);
            strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

            //add event log;
            switch(ftperror)
            {
                case DOWNLOAD_CONNNET_FAILED:
                {
                    sem_p(giSemLog);//lock
                    addEventLog(L_DOWNLOAD_CONNNET_FAILED, remoteFullPath, startTime,endTime);
                    sem_v(giSemLog);//unlock

                    //download mutex
                    sem_p(giSemDownload);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FAILED;
                    sem_v(giSemDownload);//unlock

                    break;
                }
                case DOWNLOAD_LOCAL_FILENAME_NULL:
                case DOWNLOAD_REMOTE_FILENAME_NULL:
                case DOWNLOAD_CREAET_LOCALFILE_ERROR:
                case DOWNLOAD_CONNECT_SOCKET_ERROR:
                case DOWNLOAD_PORT_MODE_ERROR:
                case DOWNLOAD_REMOTE_FILE_NOEXIST:
                {
                    sem_p(giSemLog);//lock
                    addEventLog(L_DOWNLOAD_FAILED, remoteFullPath, startTime,endTime);
                    sem_v(giSemLog);//unlock

                    //download mutex
                    sem_p(giSemDownload);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FAILED;
                    sem_v(giSemDownload);//unlock

                    break;
                }
                case DOWNLOAD_SUCCESS:
                {
                    sem_p(giSemLog);//lock
                    addEventLog(L_DOWNLOAD_SUCCESS, remoteFullPath, startTime,endTime);
                    sem_v(giSemLog);//unlock

                    //download semphore
                    sem_p(giSemDownload);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FILE_EXIST;
                    sem_v(giSemDownload);//unlock

                    break;
                }
                default:break;

            }
            //close ftp connect
            close(sockfd);
        }
    }

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
        DownloadNode * p,*p1;
		p = downloadList->next;//temporary variable always points to uploadList head pointer.
        p1 = downloadList;

		//travers daily task arry which max size is smaller than 1000;
        while( p != NULL )
        {
            sem_p(giSemDownload);//lock
            stationNum = strlen(p->state);
            taskNum = p -> taskNum;
            int len = strlen(p->state) + 1;
            state = (char*)malloc(len);
            memset(state, 0, len);
            sem_v(giSemDownload);//unlock

            /*
            *   1.according to state , count download tasks
            *   2.excute some concurrency process
            */
            int j = 0;
            pid_t *p;
            int realTaskPidMem = sizeof(pid_t)*taskNum;
            p = (pid_t *)malloc(realTaskPidMem);
            memset(p,0,realTaskPidMem);

            for( i = 0; i < stationNum; i++)
            {
                if(state[i] == 2) //if file does not exist
                {
                    p[i] = fork();//create new process.
                    if(p[i] == 0)
                    {
                        #ifdef DEBUG
                        printf("....\n");
                        #endif
                        sigleDownload(p, i);
                        return 0;
                    }

                    j++;//record real task number.
                }

                if( (j % MAX_DOWNLOAD_TASK_NUM == 0)  || ( j == taskNum ))
                {
                    wait(NULL);
                    wait(NULL);
                    #ifdef DEBUG
                    printf("waiting\n");
                    #endif
                }
            }
        }



    }
}


