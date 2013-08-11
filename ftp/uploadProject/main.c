#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/ipc.h>


#include "ftp.h"
#include "upload.h"
#include "global.h"
//#include "download.h"
#include "log.h"
#include "msem.h"



/*-----------------------------global variable in other .c file-------------------------------
*
*--------------------------------------------------------------------------------------------*/
extern  UploadList uploadList;//upload.c, file list periodically created by product center.
//extern  DownloadList downloadList;
//extern void analysisCenterMonitor();//upload.c, monitor the temporary dir in product center
//void analysisCenterCheckTask();//upload.c, check the prodcut files whether they have been uploaded
extern EventLog *elog;//record event,when an exception Occurs, like ftp connection,abnormal upload or download.


extern int giSemLog;//log semaphore
extern int giSemUpload;//upload semaphore
extern int giSemDownload;//download semaphore


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

    char    *filename,tempFilename;
    char    *analysisCenterPath,*tempAnalysisCenterPath;
	char    *productCenterPath, *tempProductCenterPath;
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
		sleep(1);


		UploadNode * p,*p1;
		p =  uploadList->next;//temporary variable always points to uploadList head pointer.
        p1 = uploadList;
        printf("upload..12.\n");
        display();

		//to traverse the upload task list, check whether there are some upload task or not.
		while( p!= NULL )
		{
            printf("upload...\n");
            sem_p(giSemUpload);//lock

            len = strlen(p->filename) + 1;
            filename = malloc(sizeof(char)*len);
            memset(filename,0,len);
            strcpy(filename, p->filename);

            len += strlen(".inmp");
            tempFilename = malloc(sizeof(char)*len);
            memset(filename,0,len);
            sprintf(tempFilename,"%s%s",p->filename, ".inmp");

            len = strlen(filename) + strlen(p->analysisCenterPath) + 1;
            analysisCenterPath = malloc(sizeof(char)*len);
            memset(analysisCenterPath,0,len);
            sprintf(analysisCenterPath,"%s%s",p->analysisCenterPath,filename);

            len = strlen(tempFilename) + strlen(analysisCenterPath) + 1;
            tempAnalysisCenterPath = malloc(sizeof(char)*len);
            memset(tempAnalysisCenterPath,0,len);
            sprintf(tempAnalysisCenterPath,"%s%s",p->analysisCenterPath,tempFilename);

            len = strlen(p->filename) + strlen(p->productCenterPath) + 1;
            productCenterPath = malloc(sizeof(char)*len);
            memset(productCenterPath,0,len);
            sprintf(productCenterPath,"%s%s",p->productCenterPath,p->filename);

            len = strlen(tempFilename) + strlen(p->productCenterPath) + 1;
            tempProductCenterPath = malloc(sizeof(char)*len);
            memset(tempProductCenterPath,0,len);
            sprintf(tempProductCenterPath,"%s%s",p->productCenterPath,tempFilename);

            uploadState = p->state;

            sem_v(giSemUpload);//unlock

            if( (uploadState == UPLOAD_FILE_EXIST) || (uploadState == UPLOAD_FILE_UNKNOWN) )
			{
                //pthread_mutex_unlock(&uploadMutex);//unlock
				//we have three times to try to connect to ftp server ,if failed. Otherwise send network error.
				int conncectTimes = 0;
				while((sockfd = connectFtpServer(p->server->ip, p->server->port, p->server->username, p->server->passwd)) <= FTP_CONNECT_FAILED_FLAG)
				{
					if( MAX_CONNECT_TIMES <= ++conncectTimes )
					{
						//to recod failed connnection time
						timer =time(NULL);
						memset(startTime, 0, 100);
						strftime( startTime, sizeof(startTime), "%Y-%m-%d %T",localtime(&timer) );

						//add event log;
						sem_p(giSemLog);//lock
						addEventLog(UPLOAD_CONNNET_FAILED, filename, startTime,"");
						sem_v(giSemLog);//unlock

                        #ifdef DEBUG
                        printf("connect error.\n");
                        #endif
						break;
					}

					//delay some time and connect ftp again.
                    //delay();
                }

                if( sockfd > FTP_CONNECT_FAILED_FLAG )
                {
                    sem_p(giSemUpload);//lock
                    p->state = UPLOAD_FILE_UPLOADING;//uploading state
                    sem_v(&giSemUpload);//unlock
                    #ifdef DEBUG
                    printf("filename = %s,\nprefix%s\n",filename,analysisCenterPath);
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
                    ftp_mkdir(tempAnalysisCenterPath);//make sure the dir exists.
                    ftperror = ftp_put(tempAnalysisCenterPath, tempProductCenterPath, sockfd);

                    // record end-upload time
                    timer =time(NULL);
                    memset(endTime, 0, 100);
                    strftime( endTime, sizeof(endTime), "%Y-%m-%d %T",localtime(&timer) );

                    //add log

                    switch(ftperror)
                    {
                        case UPLOAD_CONNNET_FAILED:
                        {
                            sem_p(giSemLog);//lock
                            addEventLog(L_UPLOAD_CONNNET_FAILED, filename, startTime,endTime);
                            sem_v(giSemLog);//unlock

                            sem_p(giSemUpload);//lock
                            p->state = UPLOAD_FILE_UPLOAD_FAILED;
                            sem_v(giSemUpload);//unlock

                            break;
                        }
                        case UPLOAD_LOCAL_FILENAME_NULL:
                        case UPLOAD_LOCAL_OPEN_ERROR:
                        case UPLOAD_DATA_SOCKET_ERROR:
                        case UPLOAD_PORT_MODE_ERROR:
                        {
                            sem_p(giSemLog);//lock
                            addEventLog(L_UPLOAD_FAILED, filename, startTime,endTime);
                            sem_v(giSemLog);//unlock

                            sem_p(giSemUpload);//lock
                            p->state = UPLOAD_FILE_UPLOAD_FAILED;
                            sem_v(giSemUpload);//unlock

                            break;
                        }
                        case UPLOAD_SUCCESS:
                        {
                            ftp_rename(tempProductCenterPath,productCenterPath);//if upload succsessfull, then rename the temporary file

                            sem_p(giSemLog);//lock
                            addEventLog(L_UPLOAD_SUCCESS, filename, startTime,endTime);
                            sem_v(giSemLog);//unlock

                            sem_p(giSemUpload);//lock
                            p->state = UPLOAD_FILE_UPLOAD_SUCCESS;
                            sem_v(giSemUpload);//unlock

                            break;
                        }
                        default:break;
                    }


                    close(sockfd);
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
                        sem_p(giSemLog);//lock
                        addEventLog(L_UPLOAD_FILE_NOEXIST, filename, endTime,endTime);
                        sem_v(giSemLog);//unlock
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_INTIME:
                    {
                        sem_p(giSemLog);//lock
                        addEventLog(L_UPLOAD_INTIME, filename, endTime,endTime);
                        sem_v(giSemLog);//unlock
                        break;
                    }
                    case UPLOAD_FILE_UPLOAD_LATE:
                    {
                        sem_p(giSemLog);//lock
                        addEventLog(L_UPLOAD_LATE, filename, endTime,endTime);
                        sem_v(giSemLog);//unlock
                        break;
                    }
                    default:break;

                }


                sem_p(giSemUpload);//lock
                p1->next=p->next;
                ////freeUploadNode(p);//free(p);
                p=p1;
                sem_v(giSemUpload);//unlock
            }

            sem_p(giSemUpload);//lock
            p1=p;
            p=p->next;
            sem_v(giSemUpload);//lock

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

		sem_p(giSemLog);//lock
		writeEventLog(UPLOAD_LOG_FILE, DOWNLOAD_LOG_FILE);
		sem_v(giSemLog);//unlock
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
        clearSpace(name);

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
            printf("fs:%o\tnode%o\n",fs->next,fsNode);


            printf("fsNode->ip:%s\n",fsNode->ip);
            printf("fs:%o\tnode%o\n",fs->next,fsNode);
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


    fclose(fp);
}



int traverseList()
{
    if(fs == NULL)
    {
        printf("fs NULL.\n");
        return -1;
    }

    FtpServer * p;


    p = fs->next;
    while(p != NULL)
    {
        printf("ip          : %s\n",p->ip);
        printf("port        : %d\n",p->port);
        printf("username    : %s\n",p->username);
        printf("passwd      : %s\n",p->passwd);

        printf("1\n");

        p = p->next;
    }

    return 0;
}


int main()
{

	// monitor analysis center,update the gloable variable uploadList, is always running
	pid_t p_analysisCenterMonitor;
	// start up uoload function,transfer data from  analysis center to products&service center
    pid_t p_upload;
	// start up download function,transfer data from  data center to analysis center
	//pid_t download;
	// log file uploading or downloading action state whether success or failed
	pid_t p_log;
	// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.
	pid_t p_timingTask;


    config("system.ini");

    traverseList();

	//initilise upload list
    initUploadlist();
    //initilise upload list
    //initDownloadloadlist();
    //initialise sem_upload,sem_download state.
    initSem();

	//create five processes,but never destroy them.
	{
        //create t analysis Center   Monitor process
        p_analysisCenterMonitor = fork();
        if( p_analysisCenterMonitor == 0)
        {
            analysisCenterMonitor();
            return 0;
        }

        //create the upload process
        p_upload = fork();
        if( p_upload == 0)
        {
            upload();
            return 0;
        }

        //create the download process
        //download = fork();
        //if( download == 0)
        //{
        //    download();
        //    return 0;
        //}

        //create the timingTask process
        p_timingTask = fork();
        if( p_timingTask == 0)
        {
            timingTask();
            return 0;
        }

        //create the log process
        p_log = fork();
        if( p_log == 0)
        {
            log();
            return 0;
        }
	}

	while(1);

	return 0;


}

