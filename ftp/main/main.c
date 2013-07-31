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
//#include "download.h"



//...........................

extern int socket_control;
extern struct TaskNode * uploadList;
extern void analysisCenterMonitor();
void analysisCenterCheckTask();

int ftpServerPort;//ftp server port
char ftpServerIP[50];//ftp server ip4 or ip6
char ftpUserName[200];//fpt server user name
char ftpPassword[50];//ftp password belong to user

//...........................


void timingTask()
{
	struct tm *st;//check current time;
	time_t t;
	int minute[5] = {0,10,25,40,55};
	int tempMin = 0;

	int zeroFlag = 0;

	struct taskMinute
	{
		int min;//record the check minute of every hour
		char traverseState;//record traverse state,0;uncheck,1:checked
	}taskMins[5];
	int i = 0;
	for( i=0;i<5;i++)//initialise the struct array;
	{
		taskMins[i].min = minute[i];
		taskMins[i].traverseState = 0;
	}

	while(1)
	{
		t=time(NULL);
		st = localtime(&t);//get current datetime
		tempMin = st->tm_sec;//catch the current minute
		sleep(1);
		printf("%s\n",ctime(&t));
		for( i = 0; i < 5; i ++)
		{
			if(tempMin == taskMins[i].min && taskMins[i].traverseState == 0)
			{
				if( i == 0)
				{
					printf("lulu.\n");//excute lulu's check module.
					analysisCenterCheckTask();
				}else
				{
					printf("yangyang.\n");//excute yangyang's monitor module.
				}

				taskMins[i].traverseState = 1;
			}

		}

		if(taskMins[4].traverseState == 1 && tempMin == 56)//initialise the traverse state when minute equals to 56,that is to say,when the last point is traversed.
		{
			for( i=0;i<5;i++)//initialise the struct array;
			{
				taskMins[i].traverseState = 0;
			}
		}

	}


}



/*
*   TaskNode member variable state: 0 created, 1 uploading, 2 success, 3 error
*
*/

void upload()
{
	int i,j;
	char oriadd[30]="/home/jung/ftp/upload/";
    char destadd[20]="upload/";
    char ori[50],dest[50];

	while(1)
	{
		TaskNode * p1;
		p1=uploadList;

		int ftperror = 0;
		while( p1 != NULL )
		{
			if(p1->state != 0)
			{
                p1=p1->next;
			}else
			{
                if ( connectFtpServer(ftpServerIP, ftpServerPort, ftpUserName, ftpPassword) > 0 )
                {
                    p1->state=1;//开始上传

					sprintf(ori,"%s%s",oriadd,p1->filename);
               		sprintf(dest,"%s%s",destadd,p1->filename);
                    if( (ftperror = ftp_put(ori, dest, socket_control) ) == 0)
                    {
                        printf("\n%s----upload success.\n",p1->filename);
                        p1->state=2;//上传成功
                    }
                    else
                    {
                        p1->state=3;//上传失败
                        printf("ftp put error\n");
                    }

                    close(socket_control);

                }else
                {
                    printf("connecting error...");
                }


			p1=p1->next;

		}
	}


//*/


	}

}









/*
	Logdata log_data[1000];
	typedef  struct
	{
		char remote_filename[1000];
		char local_filename[1000];

		int  state;        //1: undownload,2:download,3,error
	} Logdata;



typedef  struct
{
    char remote_filename[1000];
    char local_filename[1000];
    int  state;
} Logdata;
Logdata log_data[1000];

void download()
{

	while(1)
	{
		for(;i < 1000; i++)
		{
			if(log_data[i].state == 1)
			{
				if ( connectFtpServer(ftpServerIP, ftpServerPort, ftpUserName, ftpPassword) > 0 )
				{
					if( (ftperror = ftp_get(log_data[i].remote_filename, log_data[i].local_filename, socket_control) ) == 0)
					{
						 printf("\n%s----download success.\n",log_data[i].remote_filename);
		                 log_data[i].state = 2;
					}else
					{
						printf("\n%s----download fail.\n",log_data[i].remote_filename);
						log_data[i].state = 3;
					}

					close(socket_control);
				}else
		        {
		            printf("connecting error...");
					log_data[i].state = 3;
		        }
			}
		}

	}


}

*/


int initFTP()
{
	strcpy(ftpServerIP,"192.168.0.222");
	ftpServerPort = 21;
	strcpy(ftpUserName,"public");
	strcpy(ftpPassword,"123456");
}

int main()
{
	pthread_t analysisCenterMonitorTread;  // monitor analysis center,update the gloable variable uploadList, is always running
	pthread_t watchDataCenterTread;// monitor data center,update the gloable variable log_data[][]
    pthread_t uploadTread;// start up uoload function,transfer data from  analysis center to products&service center
	pthread_t downloadTread;// start up download function,transfer data from  data center to analysis center
	pthread_t logTread;// log file uploading or downloading action state whether success or failed
	pthread_t timingTaskTread;// the most important thread, the other is actived by it,expect analysisCenterMonitorTread.


	//initilise relative FTP parameters,like server ip, port,user name,password and so on.
	initFTP();


	//create seven threads,but never destroy them.
	{
		pthread_create(&analysisCenterMonitorTread,NULL,analysisCenterMonitor,(void *)NULL);//create the thread analysis Center   Monitor
		pthread_create(&uploadTread,NULL,upload,(void *)NULL); //create the thread upload
		//pthread_create(&downloadTread,NULL,download,(void *)NULL); //create the thread upload
		//pthread_create(&logTread,NULL,log,(void *)NULL); //create the thread upload
		pthread_create(&timingTaskTread,NULL,timingTask,(void *)NULL); //create the thread timing task

	}


	while(1);
}
