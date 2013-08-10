
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
#include "log.h"




/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern  UploadList uploadList;//upload.c, file list periodically created by product center.
extern  EventLog *elog;//record event,when an exception Occurs, like ftp connection,abnormal upload or download.
extern  pthread_mutex_t downloadMutex;
extern  pthread_mutex_t logMutex;

char    *tempDownloadFileSuffix;
char    *tempUploadFileSuffix;
int     maxDownloadTaskNum;；
FtpServer *fs;
UploadPath *uploadPath;
DownInfo *downInfoList;



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
					analysisCenterCheckTask();
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

    pthread_mutex_lock(&downloadMutex);

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

                //add event log;
                pthread_mutex_lock(&logMutex);;//lock
                addEventLog(DOWNLOAD_CONNNET_FAILED, remoteFullPath, startTime,"");
                pthread_mutex_unlock(&logMutex);;//unlock

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
                    pthread_mutex_lock(&logMutex);;//lock
                    addEventLog(L_DOWNLOAD_CONNNET_FAILED, remoteFullPath, startTime,endTime);
                    pthread_mutex_unlock(&logMutex);;//unlock

                    //download mutex
                    pthread_mutex_lock(&downloadMutex);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FAILED;
                    pthread_mutex_unlock(&downloadMutex);//unlock

                    break;
                }
                case DOWNLOAD_LOCAL_FILENAME_NULL:
                case DOWNLOAD_REMOTE_FILENAME_NULL:
                case DOWNLOAD_CREAET_LOCALFILE_ERROR:
                case DOWNLOAD_CONNECT_SOCKET_ERROR:
                case DOWNLOAD_PORT_MODE_ERROR:
                case DOWNLOAD_REMOTE_FILE_NOEXIST:
                {
                    pthread_mutex_lock(&logMutex);//lock
                    addEventLog(L_DOWNLOAD_FAILED, remoteFullPath, startTime,endTime);
                    pthread_mutex_unlock(&logMutex);//unlock

                    //download mutex
                    pthread_mutex_lock(&downloadMutex);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FAILED;
                    pthread_mutex_unlock(&downloadMutex);//unlock

                    break;
                }
                case DOWNLOAD_SUCCESS:
                {
                    sem_p(giSemLog);//lock
                    addEventLog(L_DOWNLOAD_SUCCESS, remoteFullPath, startTime,endTime);
                    sem_v(giSemLog);//unlock

                    //download semphore
                    pthread_mutex_lock(&downloadMutex);//lock
                    dwNode->state[nodeid] = DOWNLOAD_FILE_EXIST;
                    pthread_mutex_unlock(&downloadMutex);//unlock

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
            pthread_mutex_lock(&downloadMutex);//lock
            stationNum = strlen(p->state);
            taskNum = p -> taskNum;
            int len = strlen(p->state) + 1;
            state = (char*)malloc(len);
            memset(state, 0, len);
            pthread_mutex_unlock(&downloadMutex);//unlock

            /*
            *   1.according to state , count download tasks
            *   2.excute some concurrency process
            */
            int j = 0;
            pthread_mutex_t p[MAX_DOWNLOAD_TASK_NUM];
            int realTaskThreadMem = sizeof(pthread_mutex_t)*taskNum;
            p = (pthread_mutex_t *)malloc(realTaskThreadMem);
            memset(p,0,realTaskThreadMem);

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

                if( (j % maxDownloadTaskNum == 0)  || ( j == taskNum ))
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




/**************************************************************************************************
*    function   :   event log thread
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/
void log()
{
	initEventLog();
	while(1)
	{
		delay();

		pthread_mutex_lock(&logMutex);//lock
		writeEventLog(UPLOAD_LOG_FILE, DOWNLOAD_LOG_FILE);
		pthread_mutex_unlock(&logMutex);//unlock
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
int config(char *conf)
{
    int k=0;
    FILE *fp;
    fp = fopen(conf, "r");

    char buff[1000];
    memset(buff, 0, 1000);

    FtpServer * fsNode;

    fs=(FtpServer *)malloc(sizeof(FtpServer));
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
    }

    while(fgets(buff, 1000, fp) != NULL)
    {
        if((buff[0] == '#') || (buff[0] == '\n') )
        {
            continue;
        }

        clearSpace(buff);
        //printf("%s\n",buff);

        int i, j;
        i = j =0;
        while(buff[i++] != ':');
        char *name;
        name = (char *)malloc(i);
        memset(name, 0, i);
        strncpy(name, buff, i-1);
        j = i;


        int parNum = 0;
        if( !strcmp(name,"productCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
            memset(fsNode, 0, sizeof(FtpServer));
            printf("new node addr: \t%o\n",fsNode);

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
                        #ifdef DEBUG
                        printf("ip:%s\n",fsNode->ip);
                        #endif
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
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


                        if( buff[i-1] == '\n')
                        {
                            fsNode->passwd = (char *)malloc(1);
                            memset(fsNode->passwd, 0, 1);

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
            fsNode = NULL;

        }


        if( !strcmp(name,"dataCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
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
                        memset(fsNode->ip, 0, i-j);
                        strncpy(fsNode->ip,buff+j, i-j-1);
                        j = i;

                        #ifdef DEBUG
                        printf("fsNode->ip:%s\n",fsNode->ip);
                        #endif
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
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
                fsNode ->next = NULL;
                fs->next = fsNode;

            }else
            {
                FtpServer * tmp = fs->next->next;
                fsNode ->next = tmp;
                fs->next->next = fsNode;


            }

            fsNode = NULL;

        }

        if( !strcmp(name,"stations"))
        {
            printf("buf:%s\n",buff);
            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                char *statesionFileName;
                statesionFileName = (char *)malloc(i-j);
                memset(statesionFileName, 0, i-j);
                strncpy(statesionFileName,buff+j, i-j-1);

                j = i;
                printf("statesionFileName:%s\n",statesionFileName);

                StationList slNode;
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

                //free(station);
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
            j = i;
            printf("%s\n",tempDownloadFileSuffix);
        }

        if( !strcmp(name,"tempUploadFileSuffix"))
        {
            while(buff[i++] !='\n');
            tempUploadFileSuffix = (char *)malloc(i-j);
            memset(tempUploadFileSuffix, 0, i-j);
            strncpy( tempUploadFileSuffix, buff+j, i-j-1);
            j = i;
            printf("%s\n",tempUploadFileSuffix);
        }

        if( !strcmp(name,"maxDownloadTaskNum"))
        {
            while(buff[i++] !='\n');
            char *num = (char *)malloc(i-j);
            memset(num, 0, i-j);
            strncpy( num, buff+j, i-j-1);
            j = i;
            maxDownloadTaskNum = atoi(num);
            printf("%d\n",maxDownloadTaskNum);
        }

        if( !strcmp(name,"BDLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefix = (char *)malloc(i-j);
            memset(uploadPath->BDLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefix, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->BDLocalPathPrefix);

        }

        if( !strcmp(name,"GNSSLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefix = (char *)malloc(i-j);
            memset(uploadPath->GNSSLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefix, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->GNSSLocalPathPrefix);

        }

        if( !strcmp(name,"BDLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefixBak = (char *)malloc(i-j);
            memset(uploadPath->BDLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefixBak, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->BDLocalPathPrefixBak);
        }


        if( !strcmp(name,"GNSSLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefixBak = (char *)malloc(i-j);
            memset(uploadPath->GNSSLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefixBak, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->GNSSLocalPathPrefixBak);

        }

        if( !strcmp(name,"BDRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDRemotePathPrefix = (char *)malloc(i-j);
            memset(uploadPath->BDRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->BDRemotePathPrefix, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->BDRemotePathPrefix);

        }


        if( !strcmp(name,"GNSSRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSRemotePathPrefix = (char *)malloc(i-j);
            memset(uploadPath->GNSSRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSRemotePathPrefix, buff+j, i-j-1);
            j = i;
            printf("%s\n",uploadPath->GNSSRemotePathPrefix);

        }
    }
    fclose(fp);
}

void mergeStationList(char (*stationFileNames)[MAX_STATION_FILE_NAME_SIZE])
{
    int i = sizeof(a)/sizeof(*a);
    int j = 0;
    int totalStations = 0;
    StationNode * p = sl->next;

    for(i=0;i<sizeof(a)/sizeof(*a);i++)
    {
        while( p != NULL)
        {
            if(!strcmp(p->name, stationFileNames[i]))
            {
               totalStations += sizeof(p->sstateList)/sizeof(*(p->sstateList));
            }

            p = p->next;
        }
    }

    char (*newStaionArray)[5];
    newStaionArray = (char **)malloc(totalStations*5);
    memset(newStaionArray, 0, totalStations*5);
    totalStations = 0;
    for(i=0;i<sizeof(a)/sizeof(*a);i++)
    {
        while( p != NULL)
        {
            if(!strcmp(p->name, stationFileNames[i]))
            {
               memcpy(*newStaionArray+totalStations, *(p->sstateList), sizeof(p->sstateList));
               totalStations += sizeof(p->sstateList)/sizeof(*(p->sstateList));
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
            if(!strcmp(*newStaionArray+i, *newStaionArray+i) )
            {
                memset(*newStaionArray+i, 0, 5);
                deleteNum++;
            }
        }
    }


    StationNode * newStationNode = (StationNode *)malloc(sizeof(StationNode));
    memset(newStationNode, 0, sizeof(StationNode));

    char (*mergeStationArray)[5] = (char **)malloc((totalStations - deleteNum)*5);
    memset(mergeStationArray, 0, (totalStations - deleteNum)*5);

    for(i=0,j=0;i<totalStations-1;i++)
    {
        if(strlen(*newStaionArray+i))
        {
            memcpy(*mergeStationArray+(j++), *newStaionArray+i, 5);
        }

    }

    p = sl->next;
    sl->next = newStationNode;
    newStationNode = p;

    free(newStaionArray);
    #ifdef DEBUG
    printf("j = %d,\tdeleteNum = %d,\ttotalStations = %d\n", j,deleteNum,totalStations);
    #endif

}


void stationListReload()
{
    DownInfo *p = downInfoList->next;
    StationNode *psl = sl->next;
    int i=0,j=0;


    while( p != NULL )
    {
        i=0; j=0;
        while(psl != NULL)
        {
            if(!strcmp(p->stateList, psl->name))// not equal to
            {
                continue;
            }

            psl  = psl->next;
        }

        if( psl == NULL)
        {
            //get file number according to the character ','
            while( p->stateList[i] )
            {
                if(p->stateList[i] == ',')
                {
                    j++;
                }
                i++;
            }

            //allocate memory
            char (*stationFileName)[MAX_STATION_FILE_NAME_SIZE] = (char **)malloc((j+1)*MAX_STATION_FILE_NAME_SIZE);
            memset(stationFileName, 0, (j+1)*MAX_STATION_FILE_NAME_SIZE);

            strcpy(*stationFileName[0], strtok(p->stateList,','));
            for(i = 1; i <= j; i++)
            {
                strcpy(stationFileName[i], strtok(NULL,','));
            }

            mergeStationList(stationFileName);

            free(stationFileName);


        }
    }

}


int main()
{

	// start up download function,transfer data from  analysis center to products&service center
    pthread_t downloadTread;
	// log file uploading or downloading action state whether success or failed
	pthread_t logTread;
	// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.
	pthread_t timingTaskTread;

    config("system.ini");
    readDownloadInfo(downInfoList);
    stationListReload();
    //initDownloadlist();


    //create the thread upload
    //pthread_create(&downloadTread,NULL,upload,(void *)NULL);
    //create the thread upload
    //pthread_create(&logTread,NULL,log,(void *)NULL);
    //create the thread timing task
    //pthread_create(&timingTaskTread,NULL,timingTask,(void *)NULL);


	//while(1);
}

