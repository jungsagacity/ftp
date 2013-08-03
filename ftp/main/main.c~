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



/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern struct UploadNode * uploadList;//upload.c, file list periodically created by product center.
extern struct UploadNode * tail;//upload.c, file list periodically created by product center.
extern DownloadList downloadList[MAX_DOWNLOAD_TASK_NUM];
extern void analysisCenterMonitor();//upload.c, monitor the temporary dir in product center
void analysisCenterCheckTask();//upload.c, check the prodcut files whether they have been uploaded
extern EventLog *elog;//record event,when an exception Occurs, like ftp connection,abnormal upload or download.
extern pthread_mutex_t uploadMutex;



/*-------------------------------------ftp info-------------------------------------------------
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


/*-------------------------------------tread info-------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/
pthread_mutex_t downloadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

/*-------------------------------------FUNCTION-------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/

/**
*    function   :   initialise ftp information,like server ip, port, username, password
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/
int initFTP()
{
	// initialise data center ftp info
	dataCenterFtpServerPort = DATA_CENTER_FTP_SERVER_PORT;
	memset(dataCenterFtpServerIP, 0, 100);
	strcpy(dataCenterFtpServerIP,DATA_CENTER_FTP_SERVER_IP);
	memset(dataCenterFtpUserName, 0, 100);
	strcpy(dataCenterFtpUserName,DATA_CENTER_FTP_USER_NAME);
	memset(dataCenterFtpPassword, 0, 100);
	strcpy(dataCenterFtpPassword,DATA_CENTER_FTP_USER_PASSWORD);

	// initialise product center ftp info
	productCenterFtpServerPort = PRODUCT_CENTER_FTP_SERVER_PORT;
	memset(productCenterFtpServerIP, 0, 100);
	strcpy(productCenterFtpServerIP,PRODUCT_CENTER_FTP_SERVER_IP);
	memset(productCenterFtpUserName, 0, 100);
	strcpy(productCenterFtpUserName,PRODUCT_CENTER_FTP_USER_NAME);
	memset(productCenterFtpPassword, 0, 100);
	strcpy(productCenterFtpPassword,PRODUCT_CENTER_FTP_USER_PASSWORD);
}


/**
*    function   :   periodically check if there is task needed to be excuted;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/

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
			tempMin = st->tm_sec;//catch the current minute
			sleep(1);
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

                    pthread_mutex_lock(&downloadMutex);//lock
					module_control();
					pthread_mutex_unlock(&downloadMutex);//unlock
				}

				taskMins[i].traverseState = 1;
			}

		}

		//initialise the traverse state when minute equals to 56,that is to say,when the last minute point is traversed.
		//
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


/**
*    function   :   upload product files to product&service center ;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/
void upload()
{
	int i,j;

	char analysisCenterPathPrefix[ ]= UP_ANALYSIS_CENTER_PATH_PREFIX; // files path prex in products&servce center
    char productCenterPathPrefix[ ]= PRODUCT_CENTER_PATH_PREFIX; // files path prex in analysis center

    char *analysisCenterFullPath;
	char *productCenterFullPath;

    char filename[STD_FILENAME_SIZE] = {0};
    int uploadState = 0;

	int ftperror = 0;//ftp relative error code
	int sockfd = 0;

	//downlaod starting time and end time
	char startTime[100] = {0};
	char endTime[100] = {0};
	time_t timer;

	while(1)
	{
		UploadNode * p,*p1;
		p = uploadList;//temporary variable always points to uploadList head pointer.

		//to traverse the upload task list, check whether there are some upload task or not.
		while( p != NULL )
		{
            pthread_mutex_lock(&uploadMutex);//lock
            memset(filename,0,STD_FILENAME_SIZE);
            strcpy(filename, p->filename);
            uploadState = p->state;

            if( (uploadState == UPLOAD_FILE_EXIST) || (uploadState == UPLOAD_FILE_UNKNOWN) )
			{
                pthread_mutex_unlock(&uploadMutex);//unlock

				//we have three times to try to connect to ftp server ,if failed. Otherwise send network error.
				int conncectTimes = 0;
				while((sockfd = connectFtpServer(productCenterFtpServerIP, productCenterFtpServerPort, productCenterFtpUserName, productCenterFtpPassword)) <= FTP_CONNECT_FAILED_FLAG)
				{
					if( MAX_CONNECT_TIMES <= ++conncectTimes )
					{
						//to recod failed connnection time
						timer =time(NULL);
						memset(startTime, 0, 100);
						strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

						//add event log;
						pthread_mutex_lock(&logMutex);//lock
						addEventLog(UPLOAD_CONNNET_FAILED, filename, startTime,"");
						pthread_mutex_unlock(&logMutex);//unlock

						break;
					}
					/*
					*  delay some moment ,then connect to ftp server again.
					*  Addtionally,we are not suer to be able to  use "sleep(1)" to delay.
					*/
					int b = 0, e = 0;
                    for(b=0;b<2000;b++)
                    for(e=0;e<2000;e++);
				}

				if( sockfd > FTP_CONNECT_FAILED_FLAG )
                {
                    pthread_mutex_lock(&uploadMutex);//lock
					p->state = UPLOAD_FILE_UPLOADING;//uploading state
                    pthread_mutex_unlock(&uploadMutex);//unlock

					int len = 0;

					//allocate memory to analysisCenterFullPath and set value
					len = strlen(analysisCenterPathPrefix) + strlen(filename) + 1;
					analysisCenterFullPath = (char *)malloc(sizeof(char)*len);
					memset(analysisCenterFullPath, 0, len);
					sprintf(analysisCenterFullPath,"%s%s",analysisCenterPathPrefix,filename);

					//allocate memory to productCenterFullPath and set value
					len = strlen(productCenterFullPath) + strlen(filename) + 1;
					productCenterFullPath = (char *)malloc(sizeof(char)*len);
					memset(productCenterFullPath, 0, len);
					sprintf(productCenterFullPath,"%s%s",productCenterFullPath,filename);

                    // record starting-upload time
					timer =time(NULL);
					memset(startTime, 0, 100);
					strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

                    ftperror = ftp_put(analysisCenterFullPath, productCenterFullPath, sockfd);

                    // record end-upload time
                    timer =time(NULL);
                    memset(endTime, 0, 100);
                    strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                    //add log
                    pthread_mutex_lock(&logMutex);//lock
                    switch(ftperror)
                    {
                        case UPLOAD_CONNNET_FAILED:
                        {
                            addEventLog(L_UPLOAD_CONNNET_FAILED, filename, startTime,endTime);

                            pthread_mutex_lock(&uploadMutex);//lock
                            p->state = UPLOAD_FILE_UPLOAD_FAILED;
                            pthread_mutex_unlock(&uploadMutex);//unlock

                            break;
                        }
                        case UPLOAD_LOCAL_FILENAME_NULL:
                        case UPLOAD_LOCAL_OPEN_ERROR:
                        case UPLOAD_DATA_SOCKET_ERROR:
                        case UPLOAD_PORT_MODE_ERROR:
                        {
                            addEventLog(L_UPLOAD_FAILED, filename, startTime,endTime);

                            pthread_mutex_lock(&uploadMutex);//lock
                            p->state = UPLOAD_FILE_UPLOAD_FAILED;
                            pthread_mutex_unlock(&uploadMutex);//unlock

                            break;
                        }
                        case UPLOAD_SUCCESS:
                        {
                            addEventLog(L_UPLOAD_SUCCESS, filename, startTime,endTime);

                            pthread_mutex_lock(&uploadMutex);//lock
                            p->state = UPLOAD_FILE_UPLOAD_SUCCESS;
                            pthread_mutex_unlock(&uploadMutex);//unlock

                            break;
                        }
                        default:break;
                    }

                    pthread_mutex_unlock(&logMutex);//unlock

                    close(sockfd);
				}

            }else
            {
                pthread_mutex_unlock(&uploadMutex);//unlock
                timer =time(NULL);
                memset(endTime, 0, 100);
                strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                //add log
                pthread_mutex_lock(&logMutex);//lock
                switch(uploadState)
                {
                    case UPLOAD_FILE_NONEXIST:
                    {
                        addEventLog(L_UPLOAD_FILE_NOEXIST, filename, endTime,endTime);
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_INTIME:
                    {
                        addEventLog(L_UPLOAD_INTIME, filename, endTime,endTime);
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_LATE:
                    {
                        addEventLog(L_UPLOAD_LATE, filename, endTime,endTime);
                        break;
                    }
                    default:break;

                }
                pthread_mutex_unlock(&logMutex);//unlock

                pthread_mutex_lock(&uploadMutex);//lock
                //if the target node is the first
                if(p==uploadList)
                {
                    //at the same time the target node is the last
                    if(p==tail)
                    {
                        uploadList=NULL;
                        tail=NULL;
                    }
                    else
                        uploadList=p->next;
                }
                else
                {
                    //if the target node is at the last
                    if(p==tail)
                    {
                        tail=p1;
                        p1->next=NULL;

                    }
                    else
                        p1->next=p->next;
                }
                free(p);

                pthread_mutex_unlock(&uploadMutex);//unlock
            }

            pthread_mutex_lock(&uploadMutex);//lock
            p1=p;
            p=p->next;
            pthread_mutex_unlock(&uploadMutex);//lock

		}
	}


}


/**
*    function   :   download files from data center;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/

void download()
{
    int 	i = 0;
	char 	*src_temp;

	char	remoteFile[MAX_FTP_FILE_PATH_LEN] = {0};
	char	localFile[MAX_FTP_FILE_PATH_LEN] = {0};
	int 	downloadState = 0;

	//downlaod starting time and end time
	char startTime[100] = {0};
	char endTime[100] = {0};
	time_t timer;

	//ftp
	int sockfd = 0;
	int ftperror = 0;//ftp error code
	#ifdef DEBUG
		char 	info[1000] = {0};
	#endif
    while(1)
    {
		// sleep 1 second, and then check out whether there are some excutable task.
		sleep(1);


		//travers daily task arry which max size is smaller than 1000;
        for(; i < MAX_DOWNLOAD_TASK_NUM; i++)
        {
			//download mutex
			pthread_mutex_lock(&downloadMutex);//lock
			downloadState = downloadList[i].state;
            if( downloadState == DOWNLOAD_FILE_NONEXIST)
            {
				memset(remoteFile,0,MAX_FTP_FILE_PATH_LEN);
				memset(localFile,0,MAX_FTP_FILE_PATH_LEN);
				strcpy(remoteFile,downloadList[i].remote_filename);
				strcpy(localFile,downloadList[i].local_filename);
				pthread_mutex_unlock(&downloadMutex);//unlock


				//debug the segment progarmm code, print the first 50 task information
				#ifdef DEBUG
					if(i< DOWNLOAD_TASK_PRINT_NUM )
					{
						memset(info,0,1000);
						sprintf(info,"L:%s\tR:%s\t%d\n",localFile, remoteFile, downloadState);
						debuglog(info);
					}
				#endif

				//we have three times to try to connect to ftp server ,if failed. Otherwise send network error.
				int conncectTimes = 0;
				while((sockfd = connectFtpServer(dataCenterFtpServerIP, dataCenterFtpServerPort, dataCenterFtpUserName, dataCenterFtpPassword) )<= FTP_CONNECT_FAILED_FLAG)
				{
					printf("sockfd = %d\n",sockfd);
					if( MAX_CONNECT_TIMES <= ++conncectTimes )
					{
						//to recod failed connnection time
						timer =time(NULL);
						memset(startTime, 0, 100);
						strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

						//add event log;
						pthread_mutex_lock(&logMutex);//lock
						addEventLog(DOWNLOAD_CONNNET_FAILED, localFile, startTime,"");
						pthread_mutex_unlock(&logMutex);//unlock

						break;
					}

					/*
					*  delay some moment ,then connect to ftp server again.
					*  Addtionally,we are not suer to be able to  use "sleep(1)" to delay.
					*/
					int b = 0, e = 0;
                    for(b=0;b<2000;b++)
                    for(e=0;e<2000;e++);


				}

				if( sockfd > FTP_CONNECT_FAILED_FLAG )
				{
					//recod starting-download time
					timer =time(NULL);
					memset(startTime, 0, 100);
					strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

                    ftperror = ftp_get(remoteFile, localFile, sockfd) ;

                    //recod end-download time
                    timer =time(NULL);
                    memset(endTime, 0, 100);
                    strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                    //add event log;
                    pthread_mutex_lock(&logMutex);//lock
					switch(ftperror)
					{
                        case DOWNLOAD_CONNNET_FAILED:
                        {
                            addEventLog(L_DOWNLOAD_CONNNET_FAILED, remoteFile, startTime,endTime);

                            //download mutex
                            pthread_mutex_lock(&downloadMutex);
                            downloadList[i].state = DOWNLOAD_FAILED;
                            pthread_mutex_unlock(&downloadMutex);

                            break;
                        }
                        case DOWNLOAD_LOCAL_FILENAME_NULL:
                        case DOWNLOAD_REMOTE_FILENAME_NULL:
                        case DOWNLOAD_CREAET_LOCALFILE_ERROR:
                        case DOWNLOAD_CONNECT_SOCKET_ERROR:
                        case DOWNLOAD_PORT_MODE_ERROR:
                        case DOWNLOAD_REMOTE_FILE_NOEXIST:
                        {
                            addEventLog(L_DOWNLOAD_FAILED, remoteFile, startTime,endTime);

                            //download mutex
                            pthread_mutex_lock(&downloadMutex);
                            downloadList[i].state = DOWNLOAD_FAILED;
                            pthread_mutex_unlock(&downloadMutex);

                            break;
                        }
                        case DOWNLOAD_SUCCESS:
                        {
                            addEventLog(L_DOWNLOAD_SUCCESS, remoteFile, startTime,endTime);

                            //download mutex
                            pthread_mutex_lock(&downloadMutex);
                            downloadList[i].state = DOWNLOAD_FILE_EXIST;
                            pthread_mutex_unlock(&downloadMutex);

                            break;
                        }
                        default:break;

					}
					pthread_mutex_unlock(&logMutex);//unlock

					//close ftp connect
				    close(sockfd);
				}
            }
			pthread_mutex_unlock(&downloadMutex);/*解锁互斥量*/
        }

        i = 0;

    }
}


/**
*    function   :   event log thread
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/
void log()
{
	initEventLog();
	while(1)
	{
		int i=0,j=0;
		for(i=0;i<2000;i++)
        for(j=0;j<2000;j++);

		pthread_mutex_lock(&logMutex);//lock
		writeEventLog(UPLOAD_LOG_FILE, DOWNLOAD_LOG_FILE);
		pthread_mutex_unlock(&logMutex);//unlock


	}
}


int main()
{
	// monitor analysis center,update the gloable variable uploadList, is always running
	pthread_t analysisCenterMonitorTread;

	// start up uoload function,transfer data from  analysis center to products&service center
    pthread_t uploadTread;

	// start up download function,transfer data from  data center to analysis center
	pthread_t downloadTread;

	// log file uploading or downloading action state whether success or failed
	pthread_t logTread;

	// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.
	pthread_t timingTaskTread;


	//initilise relative FTP parameters,like server ip, port,user name,password and so on.
	initFTP();


	//create seven threads,but never destroy them.
	{
		//create the thread analysis Center   Monitor
		pthread_create(&analysisCenterMonitorTread,NULL,analysisCenterMonitor,(void *)NULL);

		//create the thread upload
		pthread_create(&uploadTread,NULL,upload,(void *)NULL);

		//create the thread upload
		pthread_create(&downloadTread,NULL,download,(void *)NULL);

		//create the thread upload
		pthread_create(&logTread,NULL,log,(void *)NULL);

		//create the thread timing task
		pthread_create(&timingTaskTread,NULL,timingTask,(void *)NULL);

	}


	while(1);
}
