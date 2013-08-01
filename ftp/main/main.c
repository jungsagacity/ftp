#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>

#include "mpthread.h"
#include "ftp.h"
#include "upload.h"
#include "global.h"
#include "download.h"
#include "log.h"



/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern struct TaskNode * uploadList;//upload.c, file list periodically created by product center.
extern DownloadList downloadList[MAX_DOWNLOAD_TASK_NUM];
extern void analysisCenterMonitor();//upload.c, monitor the temporary dir in product center 
void analysisCenterCheckTask();//upload.c, check the prodcut files whether they have been uploaded
extern EventLog *elog;//record event,when an exception Occurs, like ftp connection,abnormal upload or download.



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
			tempMin = st->tm_min;//catch the current minute
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
					module_control();					
				}

				taskMins[i].traverseState = 1;
			}

		}
		
		//initialise the traverse state when minute equals to 56,that is to say,when the last minute point is traversed.
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
	
	int ftperror = 0;//ftp relative error code
	int sockfd = 0;

	//downlaod starting time and end time
	char startTime[100] = {0};
	char endTime[100] = {0};
	time_t timer;

	while(1)
	{
		TaskNode * p;
		p = uploadList;//temporary variable always points to uploadList head pointer.
		
		//to traverse the upload task list, check whether there are some upload task or not.		
		while( p != NULL )
		{
			if( p->state == UPLOAD_FILE_EXIST )
			{

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
						addEventLog(UPLOAD_CONNNET_FAILED, p->filename, startTime,"");
												
						break;
					}
					sleep(1);						
				}	
				
				if( sockfd > FTP_CONNECT_FAILED_FLAG ) 
                {   
					// record starting-upload time
					timer =time(NULL);
					memset(startTime, 0, 100);			    
					strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );		
					
					p->state = UPLOAD_FILE_UPLOADING;//uploading state

					int len = 0;

					//allocate memory to analysisCenterFullPath and set value
					len = strlen(analysisCenterPathPrefix) + strlen(p->filename) + 1;					
					analysisCenterFullPath = (char *)malloc(sizeof(char)*len);	
					memset(analysisCenterFullPath, 0, len);	
					sprintf(analysisCenterFullPath,"%s%s",analysisCenterPathPrefix,p->filename);
					
					//allocate memory to productCenterFullPath and set value
					len = strlen(productCenterFullPath) + strlen(p->filename) + 1;
					productCenterFullPath = (char *)malloc(sizeof(char)*len);	
					memset(productCenterFullPath, 0, len);				
					sprintf(productCenterFullPath,"%s%s",productCenterFullPath,p->filename);

               		
                    if( (ftperror = ftp_put(analysisCenterFullPath, productCenterFullPath, sockfd) ) == FTP_OK)
                    {
						// record end-upload time
						timer =time(NULL);
						memset(endTime, 0, 100);			    
						strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );
	
                        p->state = UPLOAD_FILE_UPLOAD_SUCCESS;			
						//add log
						addEventLog(UPLOAD_SUCCESS, p->filename, startTime,endTime);
                    }
                    else
                    {                       
						// record end-upload time
						timer =time(NULL);
						memset(endTime, 0, 100);			    
						strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );
	
                        p->state = UPLOAD_FILE_UPLOAD_FAILED;
						
						//add log
						addEventLog(UPLOAD_FAILED, p->filename, startTime,endTime);
                        
                    }

                    close(sockfd);
				}

            }
			
			p=p->next;
				 

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

            if(downloadList[i].state == DOWNLOAD_FILE_NONEXIST)
            {
				//debug the segment progarmm code, print the first 50 task information				
				#ifdef DEBUG
					if(i< DOWNLOAD_TASK_PRINT_NUM )
					{
						memset(info,0,1000);		
						sprintf(info,"L:%s\tR:%s\t%d\n",downloadList[i].local_filename, downloadList[i].remote_filename, downloadList[i].state);
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
						addEventLog(DOWNLOAD_CONNNET_FAILED, downloadList[i].local_filename, startTime,"");
												
						break;
					}
					sleep(1);						
				}	
				
				if( sockfd > FTP_CONNECT_FAILED_FLAG )  
				{			        
					//recod starting-download time
					timer =time(NULL);
					memset(startTime, 0, 100);			    
					strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );	
				
					if( (ftperror = ftp_get(downloadList[i].remote_filename, downloadList[i].local_filename, sockfd) ) == FTP_OK)
				    {		            
						//recod end-download time				
						timer =time(NULL);
						memset(endTime, 0, 100);			    
						strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );
					
						//add event log;
						addEventLog(DOWNLOAD_SUCCESS, downloadList[i].remote_filename, startTime,endTime);
				        downloadList[i].state = DOWNLOAD_FILE_EXIST;
				    }
				    else
				    {
						//recod end-download time				
						timer =time(NULL);
						memset(endTime, 0, 100);			    
						strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );
					
						//add event log;
						addEventLog(DOWNLOAD_FAILED, downloadList[i].remote_filename, startTime,endTime);				        
				        downloadList[i].state = DOWNLOAD_FILE_DOWNLOAD_FAILED;
				    }
				
					//close ftp connect
				    close(sockfd);
				}                 
            }
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
		sleep(1);
		writeEventLog(UPLOAD_LOG_FILE, DOWNLOAD_LOG_FILE);		
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
