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
#include "utility.h"




/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern  UploadList uploadList;//upload.c, file list periodically created by product center.
extern  pthread_mutex_t uploadMutex;//upload.c
extern char *tempDownloadFileSuffix;//utility.c
extern char *tempUploadFileSuffix;//utility.c
extern char *downloadInfoFile;//utility.c
extern      FtpServer   *fs;//utility.c
extern      UploadPath *uploadPath;//utility.c






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


/**************************************************************************************************
*    function   :   upload product files to product&service center ;
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/
void upload()
{
	int     i,j;

    char    *filename,*tempFilename;
    char    *analysisCenterPath;
	char    *productCenterPath, *tempProductCenterPath;
	char    *productCenterdir;
    int     uploadState = 0;

	int     ftperror = 0;//ftp relative error code
	int     sockfd = 0;
	int     len = 0;

	//downlaod starting time and end time
	char    startTime[100] = {0};
	char    endTime[100] = {0};
	time_t  timer;

	while(1)
	{
		UploadNode * p,*p1;
		p =  uploadList->next;//temporary variable always points to uploadList head pointer.
        p1 = uploadList;

        delay();
		//to traverse the upload task list, check whether there are some upload task or not.
		while( p!= NULL )
		{
            pthread_mutex_lock(&uploadMutex);//lock

            len = strlen(p->filename) + 1;
            filename = malloc(sizeof(char)*len);
            memset(filename,0,len);
            strcpy(filename, p->filename);


            len = len + strlen(TEMP_SUFFIX) + 1;

            tempFilename = malloc(sizeof(char)*len);
            memset(tempFilename,0,len);
            sprintf(tempFilename,"%s%s",p->filename, TEMP_SUFFIX);
            //printf("tempFilename: %s\n",tempFilename);

            len = strlen(filename) + strlen(p->analysisCenterPath) + 1;
            analysisCenterPath = malloc(sizeof(char)*len);
            memset(analysisCenterPath,0,len);
            sprintf(analysisCenterPath,"%s%s",p->analysisCenterPath,filename);
            //printf("analysisCenterPath: %s\n",analysisCenterPath);


            len = strlen(p->filename) + strlen(p->productCenterPath) + 1;
            productCenterPath = malloc(sizeof(char)*len);
            memset(productCenterPath,0,len);
            sprintf(productCenterPath,"%s%s",p->productCenterPath,p->filename);

            len = strlen(p->productCenterPath) + 1;
            productCenterdir = malloc(sizeof(char)*len);
            memset(productCenterdir,0,len);
            strcpy(productCenterdir, p->productCenterPath);

            len = strlen(tempFilename) + strlen(p->productCenterPath) + 1;
            tempProductCenterPath = malloc(sizeof(char)*len);
            memset(tempProductCenterPath,0,len);
            sprintf(tempProductCenterPath,"%s%s",p->productCenterPath,tempFilename);

            uploadState = p->state;

            pthread_mutex_unlock(&uploadMutex);//unlock

            if( (uploadState == UPLOAD_FILE_EXIST) || (uploadState == UPLOAD_FILE_UNKNOWN) )
			{
                //pthread_mutex_unlock(&uploadMutex);//unlock
				//we have three times to try to connect to ftp server ,if failed. Otherwise send network error.
				int conncectTimes = 0;
				//while((sockfd = connectFtpServer(p->server->ip, p->server->port, p->server->username, p->server->passwd)) <= FTP_CONNECT_FAILED_FLAG)
				while( (sockfd = connectFtpServer(p->server->ip, p->server->port, p->server->username, p->server->passwd)) <= 0)
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

                        #ifdef DEBUG
                        printf("connect error.\n");
                        #endif
						break;
					}

					//delay some time and connect ftp again.
                    delay();
                }

                if( sockfd > FTP_CONNECT_FAILED_FLAG )
                {
                    pthread_mutex_lock(&uploadMutex);//lock
                    p->state = UPLOAD_FILE_UPLOADING;//uploading state
                    pthread_mutex_unlock(&uploadMutex);//unlock
                    #ifdef DEBUG
                    printf("filename = %s,\tpath:%s\n",filename,analysisCenterPath);
                    #endif

                    // record starting-upload time
                    timer =time(NULL);
                    memset(startTime, 0, 100);
                    strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

                    /*
                    *   1.check out whether the file's father dir do exist or not.
                    *   2.if exist, then upload the specifed file;
                    *   3.else, make dir and then upload the file;
                    *   4.rename the file if upload it completely.
                    */
                    //printf("%s,\t%s\n",analysisCenterPath,tempProductCenterPath);
                    ftp_mkdir(productCenterdir, sockfd);//make sure the dir exists.
                    ftperror = ftp_put(analysisCenterPath, tempProductCenterPath, sockfd);
                    //ftperror = ftp_put("/home/jung/2.txt", "upload/fsda.txt", sockfd);
                    //delay();
                    //break;

                    // record end-upload time
                    timer =time(NULL);
                    memset(endTime, 0, 100);
                    strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                    //add log

                    switch(ftperror)
                    {
                        case UPLOAD_CONNNET_FAILED:
                        {
                            pthread_mutex_lock(&logMutex);//lock
                            addEventLog(L_UPLOAD_CONNNET_FAILED, filename, startTime,endTime);
                            pthread_mutex_unlock(&logMutex);//unlock

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
                            pthread_mutex_lock(&logMutex);//lock
                            addEventLog(L_UPLOAD_FAILED, filename, startTime,endTime);
                            pthread_mutex_unlock(&logMutex);//unlock

                            pthread_mutex_lock(&uploadMutex);//lock

                            if(p->state == UPLOAD_FILE_UNKNOWN)
                            {
                                freeUploadNode(p);//free(p);
                                p=p1;
                            }
                            else
                                p->state = UPLOAD_FILE_UPLOAD_FAILED;
                            pthread_mutex_unlock(&uploadMutex);//unlock

                            break;
                        }
                        case UPLOAD_SUCCESS:
                        {
                            ftp_rename(tempProductCenterPath,productCenterPath, sockfd);//if upload succsessfull, then rename the temporary file

                            pthread_mutex_lock(&logMutex);//lock
                            addEventLog(L_UPLOAD_SUCCESS, filename, startTime,endTime);
                            pthread_mutex_unlock(&logMutex);//unlock

                            pthread_mutex_lock(&uploadMutex);//lock
                            if(p->state == UPLOAD_FILE_UNKNOWN)
                            {
                                freeUploadNode(p);//free(p);
                                p=p1;
                            }
                            else
                                p->state = UPLOAD_FILE_UPLOAD_SUCCESS;
                            pthread_mutex_unlock(&uploadMutex);//unlock

                            break;
                        }
                        default:break;
                    }


                    close(sockfd);
                    delay();
                }
            }
            else if((uploadState == UPLOAD_FILE_NONEXIST) || (uploadState == UPLOAD_FILE_UPLOAD_INTIME) || (uploadState == UPLOAD_FILE_UPLOAD_LATE))
            {

                timer =time(NULL);
                memset(endTime, 0, 100);
                strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                switch(uploadState)
                {
                    case UPLOAD_FILE_NONEXIST:
                    {
                        pthread_mutex_lock(&logMutex);//lock
                        addEventLog(L_UPLOAD_FILE_NOEXIST, filename, endTime,endTime);
                        pthread_mutex_unlock(&logMutex);//unlock
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_INTIME:
                    {
                        pthread_mutex_lock(&logMutex);//lock
                        addEventLog(L_UPLOAD_INTIME, filename, endTime,endTime);
                        pthread_mutex_unlock(&logMutex);//unlock
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_LATE:
                    {
                        pthread_mutex_lock(&logMutex);//lock
                        addEventLog(L_UPLOAD_LATE, filename, endTime,endTime);
                        pthread_mutex_unlock(&logMutex);//unlock
                        break;
                    }
                    default:break;

                }


                pthread_mutex_lock(&uploadMutex);//lock
                //p1->next=p->next;
                freeUploadNode(p);//free(p);
                p=p1;
                pthread_mutex_unlock(&uploadMutex);//unlock
            }

            pthread_mutex_lock(&uploadMutex);//lock
            p1=p;
            p=p->next;
            pthread_mutex_unlock(&uploadMutex);//unlock

        }

    }

}


int main()
{

	// monitor analysis center,update the gloable variable uploadList, is always running
	pthread_t analysisCenterMonitorTread;
	// start up uoload function,transfer data from  analysis center to products&service center
    pthread_t uploadTread;
	// log file uploading or downloading action state whether success or failed

	pthread_t timingTaskTread;

    config("system.ini");
    //traverseList();
    initUploadlist();

    //create the thread analysis Center   Monitor
    pthread_create(&analysisCenterMonitorTread,NULL,analysisCenterMonitor,(void *)NULL);

    //create the thread upload
    pthread_create(&uploadTread,NULL,upload,(void *)NULL);

    //create the thread timing task
    pthread_create(&timingTaskTread,NULL,timingTask,(void *)NULL);


	while(1);
}

