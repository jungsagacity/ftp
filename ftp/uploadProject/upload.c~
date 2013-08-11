#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/inotify.h>
#include <malloc.h>
#include <time.h>
#include <pthread.h>

#include "global.h"
#include "upload.h"



/********  GLOBAL VARIBALES  ********/
UploadList uploadList;
//head of the uploadlist
extern FtpServer * fs;
extern UploadPath * uploadPath;

pthread_mutex_t uploadMutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef DEBUG

/**
 *      function    :   get current time (strings  ,for writing log)
        return      :   {char *}    like Wed Jul 31 15:11:14 2013
**/

char * getchartime()
{
	time_t t;
	t=time(NULL);
	return ctime(&t);
}

/**
 *      function    :   show each node of the list
        return      :   void
**/

void display()
{
    UploadNode *p;
    p=uploadList;
    while(p!=NULL)
    {
        printf("链表：%s\t%s\t%s\t%d\t%d\n",p->filename,p->analysisCenterPath,p->productCenterPath,p->state,p->type);
        p=p->next;
    }
}

/**
 *      function    :   write log ,include upload state ,upload file's name,time
        return      :   void
**/

void log_checktask(char * name,char * a)
{
	struct tm *t;
	char * curtime;
	int k;
	t=gettime();
	curtime=getchartime();
	int year=t->tm_year+1900;
	int month=t->tm_mon+1;
	char logdir[CHECKTASK_LOG_PATH_SIZE];
	sprintf(logdir,"%s%04d%02d%s",CHECKTASK_LOG_PATH,year,month,CHECKTASK_LOG_TYPE);
	FILE *fp;
	fp=fopen(logdir,"a+");
	if(fp==NULL)
	 exit(1);
	fprintf(fp,"%s\t%s\t%s\n",a,name,curtime);
//	fprintf(fp,"%s\n",a);
	fclose(fp);
}

#endif


/**
 *      function    :   get current time (to get year,month ,day ... indivisually)
        return      :   {struct tm *}
                        struct tm {
                                     int tm_sec;     // seconds after the minute: [0 - 60]
                                     int tm_min;     // minutes after the hour: [0 - 59]
                                     int tm_hour;    // hours after midnight: [0 - 23]
                                     int tm_mday;    // day of the month: [1 - 31]
                                     int tm_mon;     // months since January: [0 - 11]
                                     int tm_year;    // years since 1900
                                     int tm_wday;    // days since Sunday: [0 - 6]
                                     int tm_yday;    // days since January 1: [0 - 365]
                                     int tm_isdst;   // daylight saving time flag: <0, 0, >0
                                    };
**/

struct tm * gettime()
{
	time_t t;
	t=time(NULL);
	return localtime(&t);
}

/**
 *      function    :   initialize the list,create the head node
 *      para        :   void
        return      :   void
**/

void initUploadlist()
{
    uploadList = (UploadNode*)malloc(sizeof(UploadNode));
    memset(uploadList,0,sizeof(UploadNode));
    uploadList -> next = NULL;
}

/**
 *      function    :   insert node into the tail of the uploadlist
 *      para        :   {UploadNode *}
        return      :   void
**/

void insertlist(UploadNode * p0)
{
    #ifdef DEBUG
    printf("insert\n");
    #endif
    pthread_mutex_lock(&uploadMutex);//lock
    #ifdef DEBUG
    printf("insert list metux.\n");
    #endif
    //insert into the end of the list
    p0->next=uploadList->next;
    uploadList->next=p0;
    #ifdef DEBUG
 //   display();
    #endif
    pthread_mutex_unlock(&uploadMutex);//unlock
    printf("insert_end\n");
}

/**
 *      function    :   when file is not exist ,create one
 *      para        :   {char * name}   like "ACC03957.sum.Z"
                        {int type}      1:BD    2:GNSS
        return      :   void
**/

void create_nonexist_node(char * name,int type)
{
        UploadNode *p0;
		p0=(UploadNode *)malloc(sizeof(UploadNode));
		int len = strlen(name)+1;
		p0->filename = (char *)malloc(len);
		memset(p0->filename,0, len);
		strcpy(p0->filename,name);
		//state=UPLOAD_FILE_NONEXIST ,  not upload in time

        //BD
        if(type==1)
        {
            int len2=strlen(uploadPath->BDLocalPathPrefix)+1;
            p0->analysisCenterPath=(char *)malloc(len2);
            memset(p0->analysisCenterPath,0, len2);
            strcpy(p0->analysisCenterPath,uploadPath->BDLocalPathPrefix);
        }
        //GNSS
        else
        {
            int len2=strlen(uploadPath->GNSSLocalPathPrefix)+1;
            p0->analysisCenterPath=(char *)malloc(len2);
            memset(p0->analysisCenterPath,0, len2);
            strcpy(p0->analysisCenterPath,uploadPath->GNSSLocalPathPrefix);
        }

        p0->productCenterPath = (char *)malloc(1);
        memset(p0->productCenterPath,0, 1);

 		p0->state=UPLOAD_FILE_NONEXIST;
 		p0->type = type;
 		pthread_mutex_unlock(&uploadMutex);//unlock
        insertlist(p0);
        pthread_mutex_lock(&uploadMutex);//lock
}


/**
 *      function    :   search the list according to the name to check the file's state
 *      para        :   {char * name}   like "ACC03957.sum.Z"
                        {int type}      1:BD    2:GNSS
        return      :   void
**/

void checkfile(char * name,int type)
{
    pthread_mutex_lock(&uploadMutex);//lock

	/*
	the list is empty which obvious means the file is not upload
	create a new node record the filename and state=4
	to inform the notify center
	*/
	if(uploadList->next==NULL)
	{
        #ifdef DEBUG
        printf("文件未创建\n");
        log_checktask(name,"文件未创建");
        #endif
        //create a new node ,the insert into list
        create_nonexist_node(name ,type);
	}
	/*
	the list is not empty
	go through the list to find the file accord to the name
	change the related state
	*/
    else
    {
        //pthread_mutex_lock(&uploadMutex);//lock
        UploadNode *p1;

        //p2=uploadList;
        p1=uploadList->next;

        //go through the list until find the node or at the end
        while( p1!=NULL )
        {
            char * filename = p1->filename;
            int state = p1->state;

            if( (strcmp(name,filename)!=0) || (p1->type !=type))
            {
                p1=p1->next;
            }
            else
            {

                //find the node you are searching for,change the state
                /*
                state=UPLOAD_FILE_UPLOAD_SUCCESS means uploaded success
                change the state=UPLOAD_FILE_UPLOAD_INTIME,in time
                */
                if(state==UPLOAD_FILE_UPLOAD_SUCCESS)
                {
                    state=UPLOAD_FILE_UPLOAD_INTIME;
                    #ifdef DEBUG
                    log_checktask(name,"文件上传成功");
                    printf("%s:文件上传成功\n",name);
                    #endif
                }

                /*
                state=1 means the file is uploading,not finish
                change the state=UPLOAD_FILE_UPLOAD_LATE,not in time
                */
                else if(p1->state==UPLOAD_FILE_UPLOADING)
                {
                    p1->state=UPLOAD_FILE_UPLOAD_LATE;
                    #ifdef DEBUG
                    printf("%s:state=1文件正在上传\n",name);
                    log_checktask(name,"文件正在上传");
                    #endif
                }

                /*
                others like state=UPLOAD_FILE_EXIST or UPLOAD_FILE_UPLOAD_FAILED,means the file is not uploaded,or upload fails
                change the state=UPLOAD_FILE_UPLOAD_LATE,not in time
                */
                else if(p1->state==UPLOAD_FILE_EXIST)
                {
                    p1->state=UPLOAD_FILE_UNKNOWN;
                    #ifdef DEBUG
                    printf("%s:state=0文件为上传\n",name);//可以继续调用ftp
                    log_checktask(name,"文件未上传");
                    #endif
                }
                else
                {
                    p1->state=UPLOAD_FILE_UPLOAD_LATE;
                    #ifdef DEBUG
                    printf("%s:文件上传失败\n",name);//可以继续调用ftp
                    log_checktask(name,"文件上传失败");
                    #endif
                }

                break;
            }

        }
        /*
        go through the list can't find the node
        means file is not producted,not in time
        create a new node record the filename and state=4
        to inform the notify center
        */
        if(p1 == NULL)
        {
            //create a new node ,insert into list
            create_nonexist_node(name , type);
            #ifdef DEBUG
            printf("%s:文件未生成\n",name);//待定
            //display();
            #endif
        }
    }


    pthread_mutex_unlock(&uploadMutex);//unlock
}


/**
 *      function    :   delete the node,and free the memory
 *      para        :   {UploadNode * up}   the point to the node you want to delete
        return      :   void
**/
void  freeUploadNode(UploadNode * up)
{
    UploadNode * p =uploadList,*q = uploadList->next;
    while(q != up && q!=NULL)
    {
        p=q;
        q=q->next;
    }
    if(q==NULL)
    {
        #ifdef DEBUG
        printf("The node is not exist\n");
        #endif
    }
    else
    {
        p->next=q->next;

        free(q->productCenterPath);
        free(q->analysisCenterPath);
        free(q->filename);

        free(q);
        printf("free\n");
    }

}

/**
 *      function    :   To determine whether a leap year
 *      para        :   {int}  like 2013
        return      :   {int}   1:leap year
                                0:not a leap year
**/

int IsLeapYear(int year)
{
	if ((year%400==0) ||(year%4==0&& year%100!=0))
		return 1;
	else
        return 0;
}

/**
 *      function    :   count the days in the year
 *      para        :   {int year,int month,int day}    like 2013,07,31
        return      :   {int}
**/

int GetDaysOfMonth(int year,int month,int day)
{
    int i=0;
    //count the days before the current month at the current year
    switch(month-1)
    {
        case 11:    i+=30;
        case 10:    i+=31;
        case  9:    i+=30;
        case  8:    i+=31;
        case  7:    i+=31;
        case  6:    i+=30;
        case  5:    i+=31;
        case  4:    i+=30;
        case  3:    i+=31;
        //depends on the leap year
        case  2:    i+=((IsLeapYear(year))?29:28);
        case  1:    i+=31;
    }
    //add the current month's day
    return i+day-1;
}

/**
 *      function    :   count 北斗周
 *      para        :   {int year,int month,int day}    like 2013,07,31
        return      :   {int}
**/

int BTS_Time(int year,int month,int day)
{
    //days records the days between two dates
    int days=0,j;
    int std_year=BEIDOU_YEAR;
    int std_month=BEIDOU_MONTH;
    int std_day=BEIDOU_DAY;
    for(j=std_year;j<year;j++)
        days+=(IsLeapYear(j)?366:365);
    days+=GetDaysOfMonth(year,month,day);
    //divide 7 to count the weeks
    return (days/7+1);
}

/**
 *      function    :   if the folder is not exist,create it
 *      para        :   {char *filePath}
        return      :   {int}       -1: wrong para
                                     0:create success
**/

int fileIsExist(char *filePath)
{
    printf("filePath: %s\n",filePath);
    int len =  strlen(filePath);
    char *path;
    if( len == 0)
    {
        return -1;
    }

    path = (char *)malloc(len+1);
    memset(path,0,len+1);
    sprintf(path,"%s",filePath);
    printf("path:%s\n",path);
    int p;
    //create each folder
    for( p = 1; p <= len; p++ )
    {
        if( path[p] == '/')
        {
            path[p] = 0;
            if( access(path, 0) != 0)
            {
                if( mkdir(path, 0777) == -1 )
                {
                    #ifdef DEBUG
                    printf("make dir %s error",path);
                    #endif
                    return -1;
                }
            }

             path[p] = '/';
        }

    }

    free(path);

    return 0;
}

/**
 *      function    :   copy the file to the backup folder
 *      para        :   {char * filename,int flg}
        return      :   {void}
**/

void copyfile(char * filename,int type,char * dir)
{
	//char * filename = p0->filename;
	char * current_path;
	char * backup_path;
	char * filepath;
    char command[COMMAND_SIZE] = {0};

    #ifdef DEBUG
	printf("filename:%s\n",filename);
	#endif

	//the file is named after BD week, from the third to the sixth
	if((strstr(filename,"isa")==0)&&(strstr(filename,"dcb")==0))
	{
		strncpy(dir, filename+3, 4);
		#ifdef DEBUG
        printf("if\n");
		printf("%s\n",dir);
		#endif
	}
	//the file is named after year and month, from the third to the eighth
	else
	{
		strncpy(dir,filename+3,6);
		#ifdef DEBUG
		printf("else\n");
		printf("%s\n",dir);
		#endif
	}

    //the file is in the BD folder
    if(type==1)
	{
        current_path=uploadPath->BDLocalPathPrefix;
        backup_path=uploadPath->BDLocalPathPrefixBak;
	}
	//the file is in the GNSS folder
	else
	{
        current_path=uploadPath->GNSSLocalPathPrefix;
        backup_path=uploadPath->GNSSLocalPathPrefixBak;
	}

	int lenth = strlen(backup_path)+strlen(dir)+strlen(PATH_SUFFIX)+1;
	filepath = (char * )malloc(lenth);
	memset(filepath,0,lenth);
	sprintf(filepath,"%s%s%s",backup_path,dir,PATH_SUFFIX);

	#ifdef DEBUG
	printf("filepath:%s\n",filepath);
	#endif

	if(fileIsExist(filepath)==0)
    {

        sprintf(command,"cp %s%s %s%s",current_path,filename,backup_path,dir);

        #ifdef DEBUG
        printf("command:%s\n",command);
        #endif

        FILE *pf;
        if((pf=popen(command,"r"))==NULL)
        {
            //move failed
            #ifdef _DEBUG
            perror("移动失败");
            #endif
        }
        pclose(pf);
    }

    free(filepath);
}

/**
 *      function    :   search the list according to the name to check the node is exist
 *      para        :   {char * name}   like "ACC03957.sum"
                        {int type}      1:BD    2:GNSS
        return      :   {int}           0:exist     1:nonexist
**/

int nodeIsExist(char * name,int type)
{
    pthread_mutex_lock(&uploadMutex);//lock

	if(uploadList->next==NULL)
	{
        #ifdef DEBUG
        printf("节点不存在\n");
        #endif
        pthread_mutex_unlock(&uploadMutex);//unlock
        return 1;
	}
	/*
	the list is not empty
	go through the list to find the file accord to the name
	change the related state
	*/
    else
    {
        //pthread_mutex_lock(&uploadMutex);//lock
        UploadNode *p1;

        //p2=uploadList;
        p1=uploadList->next;

        //go through the list until find the node or at the end
        while( p1!=NULL )
        {
            char * filename = p1->filename;
            printf("filename:%s\t name:%s\n",filename,name);
            if( (strstr(filename,name)!=0) && (p1->type ==type))
            {
                printf("0000000000000000\n");
                pthread_mutex_unlock(&uploadMutex);//unlock
                return 0;

            }

            p1=p1->next;
        }
        printf("111111111111111111\n");
        pthread_mutex_unlock(&uploadMutex);//unlock
        return 1;

    }
}


/**
 *      function    :   This method does the dirty work of determining what happened,
                        Then allows us to act appropriately
                        Here we act to only two things:IN_CLOSE_WRITE,IN_DELETE
                        IN_CLOSE_WRITE:It means the product is exixt,then we can compress then upload
                        IN_DELETE:It means the compressoin is done,insert into uploadlist.
 *      para        :   {struct inotify_event *event}
        return      :   {void}
**/

static void _inotify_event_handler(struct inotify_event *event)
{
	/* Perform event dependent handler routines */
	/* The mask is the magic that tells us what file operation occurred */
    if(strstr(event->name,UNIX_Z)==0)
    {
        /*File was closed */
        if(event->mask & IN_CLOSE_WRITE)
        {
            #ifdef DEBUG
            printf("wd:%d",event->wd);
            printf("IN_CLOSE_WRITE\n");
            printf("event->name: %s\n", event->name);
            #endif

            char command[COMMAND_SIZE]={0};
            FILE *pf;
            //get the command to compress the file with unix.z
            if(event->wd==1)
                sprintf(command,"compress -f %s%s",uploadPath->BDLocalPathPrefix,event->name);
            else
                sprintf(command,"compress -f %s%s",uploadPath->GNSSLocalPathPrefix,event->name);
            //execute the command of compression
            if((pf=popen(command,"r"))==NULL)
            {
                //compression failed
                #ifdef _DEBUG
                perror("压缩失败");
                #endif
            }
            pclose(pf);

        }

        /* File was deleted means the compression is over*/
        if(event->mask & IN_DELETE)
        {
            #ifdef DEBUG
            printf("IN_DELETE\n");
            printf("event->name: %s\n", event->name);
            #endif
            int type = event-> wd;
            char name[STD_FILENAME_SIZE]="0";
            strcpy(name,event->name);
            if(nodeIsExist(name,type)==1)
            {
                char * dir;
                UploadNode *p0;
                p0=(UploadNode *)malloc(sizeof(UploadNode));

                int len1 = strlen(name)+strlen(UNIX_Z)+1;
                p0->filename=(char *)malloc(len1);
                memset(p0->filename,0, len1);
                sprintf(p0->filename,"%s%s",name,UNIX_Z);
                printf("filename:%s\n",p0->filename);
                //move the file to the backup folder

                dir = (char *)malloc(10);
                memset(dir,0,10);

                //move the file to the backup folder
                copyfile(p0->filename,type,dir);

                //BEIDOU
                if(type==1)
                {
                    int len2=strlen(uploadPath->BDLocalPathPrefix)+1;
                    p0->analysisCenterPath=(char *)malloc(len2);
                    memset(p0->analysisCenterPath,0, len2);
                    strcpy(p0->analysisCenterPath,uploadPath->BDLocalPathPrefix);

                    int len3 = strlen(uploadPath->BDRemotePathPrefix)+strlen(dir)+strlen(PATH_SUFFIX)+1;
                    p0->productCenterPath=(char *)malloc(len3);
                    memset(p0->productCenterPath,0, len3);
                    sprintf(p0->productCenterPath,"%s%s%s",uploadPath->BDRemotePathPrefix,dir,PATH_SUFFIX);
                }
                //GNSS
                else
                {
                    int len2=strlen(uploadPath->GNSSLocalPathPrefix)+1;
                    p0->analysisCenterPath=(char *)malloc(len2);
                    memset(p0->analysisCenterPath,0, len2);
                    strcpy(p0->analysisCenterPath,uploadPath->GNSSLocalPathPrefix);

                    int len3 = strlen(uploadPath->GNSSRemotePathPrefix)+strlen(dir)+strlen(PATH_SUFFIX)+1;
                    p0->productCenterPath=(char *)malloc(len3);
                    memset(p0->productCenterPath,0, len3);
                    sprintf(p0->productCenterPath,"%s%s%s",uploadPath->GNSSRemotePathPrefix,dir,PATH_SUFFIX);
                }



                p0->state=UPLOAD_FILE_EXIST;
                p0->type = type;
                p0->server=fs->next;
                #ifdef DEBUG
                printf("filename:%s\n",p0->filename);
                printf("传入：%s\n",p0->filename);
                log_checktask(p0->filename,"文件创建");
                #endif
                //create a new node
                insertlist(p0);
            }



    }

    }
}

/**
 *      function    :   the main function of the monitor
                        set monitor condition,begin monitor,send information
 *      para        :   {void}
        return      :   {void}
**/

void analysisCenterMonitor()
{

    #ifdef DEBUG
    printf("analysisCenterMonitor\n");
    #endif

    //strcut and variables to store the monitor events
    //   initUploadlist();
	unsigned char buf[BUF_MAX] = {0};
	struct inotify_event *event = {0};
	int fd = inotify_init();
	/*
	set monitor path with the uploadPath->BDLocalPathPrefix or 2
	set the monitor condition with INOTIFY_EVENTS
	*/
	int wd_1 = inotify_add_watch(fd,uploadPath->BDLocalPathPrefix ,INOTIFY_EVENTS);
	int wd_2 = inotify_add_watch(fd,uploadPath->GNSSLocalPathPrefix ,INOTIFY_EVENTS);
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

    //nonstoped ,when events orrured , call _inotify_event_handler(),take act to each event
	while(1)
	{
		if (select(fd + 1, &fds, NULL, NULL, NULL) > 0)
		{
			int len, index = 0;
			while (((len = read(fd, &buf, sizeof(buf))) < 0) && (errno == EINTR));
			while (index < len)
			{
				event = (struct inotify_event *)(buf + index);
				//if the events we monitored happened
				_inotify_event_handler(event);
				index += sizeof(struct inotify_event) + event->len;
			}
		}
	}
	//cancel the monitor
	inotify_rm_watch(fd, wd_1);
	inotify_rm_watch(fd, wd_2);

}



/**
 *      function    :   form the name of every hour's checking task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d,int hr}
        return      :   {void}
**/

void hourtask(int wwww,int d,int hr)
{
	char bd_std_filename[STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="ACI";
	char post_filename[POST_FILENAME_SIZE]=".irt.Z";
    sprintf(bd_std_filename,"%s%04d%d_%02d%s",pre_filename,wwww,d,hr,post_filename);
    //standard file name:ACIwwwwd_HR.irt.Z
    checkfile(bd_std_filename,BD);

    int gps_week = wwww + GPS_WEEK;
	char gps_std_filename[STD_FILENAME_SIZE]={0};
    sprintf(gps_std_filename,"%s%04d%d_%02d%s",pre_filename,gps_week,d,hr,post_filename);
    //standard file name:ACIwwwwd_HR.irt.Z
    checkfile(gps_std_filename,GPS);


}

/**
 *      function    :   form the name of every 6 hour's checking task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d,int hr}
        return      :   {void}
**/

void hour6task(int wwww,int d,int hr)
{
	char bd_std_filename[3][STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="ACU";
	char post_filename[3][POST_FILENAME_SIZE]={".sp3.Z",".erp.Z",".tro.Z"};
	int i=0;
	for(i=0;i<3;i++)
	{
		sprintf(bd_std_filename[i],"%s%04d%d_%02d%s",pre_filename,wwww,d,hr,post_filename[i]);
		//standard file name:ACUwwwwd_HR.sp3.Z,ACUwwwwd_HR.erp.Z,ACUwwwwd_HR.tro.Z
		checkfile(bd_std_filename[i],BD);
	}

    int gps_week = wwww + GPS_WEEK;
	char gps_std_filename[3][STD_FILENAME_SIZE]={0};
	for(i=0;i<3;i++)
	{
        sprintf(gps_std_filename[i],"%s%04d%d_%02d%s",pre_filename,gps_week,d,hr,post_filename[i]);
        //standard file name:ACIwwwwd_HR.irt.Z
        checkfile(gps_std_filename[i],GPS);
    }
}

/**
 *      function    :   form the name of every day's checking task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d}
        return      :   {void}
**/

void daytask(int wwww,int d)
{
	char bd_std_filename[3][STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="ACR";
	char post_filename[3][POST_FILENAME_SIZE]={".sp3.Z",".clk.Z",".erp.Z"};
	int i=0;
	for(i=0;i<3;i++)
	{
        sprintf(bd_std_filename[i],"%s%04d%d%s",pre_filename,wwww,d,post_filename[i]);
        //standard file name:ACUwwwwd.sp3.Z,ACUwwwwd.clk.Z,ACUwwwwd.erp.Z
        checkfile(bd_std_filename[i],BD);
	}

    int gps_week = wwww + GPS_WEEK;
	char gps_std_filename[3][STD_FILENAME_SIZE]={0};
	for(i=0;i<3;i++)
	{
        sprintf(gps_std_filename[i],"%s%04d%d%s",pre_filename,gps_week,d,post_filename[i]);
        //standard file name:ACIwwwwd_HR.irt.Z
        checkfile(gps_std_filename[i],GPS);
    }
}

/**
 *      function    :   form the name of every day's checking task
                        different form the daytask() in checking time and name
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d}
        return      :   {void}
**/

void daytask1(int wwww,int d)
{
	char bd_std_filename[STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="ACR";
	char post_filename[POST_FILENAME_SIZE]=".ion.Z";
    sprintf(bd_std_filename,"%s%04d%d%s",pre_filename,wwww,d,post_filename);
    //standard file name:ACRwwwwd.ion.Z
    checkfile(bd_std_filename,BD);

    int gps_week = wwww + GPS_WEEK;
    char gps_std_filename[STD_FILENAME_SIZE]={0};
    sprintf(gps_std_filename,"%s%04d%d%s",pre_filename,gps_week,d,post_filename);
    //standard file name:ACRwwwwd.ion.Z
    checkfile(gps_std_filename,GPS);
}

/**
 *      function    :   form the name of every week's check task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww}
        return      :   {void}
**/

void weektask(int wwww)
{
	char bd_std_filename[7][STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="AAC";
	char post_filename[7][POST_FILENAME_SIZE]={"7.sp3.Z","7.clk.Z","7.snx.Z","7.erp.Z","7.tro.Z","7.ion.Z","7.sum.Z"};
	int i=0;
	for(i=0;i<7;i++)
	{
        sprintf(bd_std_filename[i],"%s%04d%s",pre_filename,wwww,post_filename[i]);
        /*
        standard file name:
        AACwwww7.sp3.Z,AACwwww7.clk.Z,AACwwww7.snx.Z
        AACwwww7.erp.Z,AACwwww7.tro.Z,AACwwww7.ion.Z,AACwwww7.sum.Z
        */
        checkfile(bd_std_filename[i],BD);
	}

	int gps_week = wwww + GPS_WEEK;
    char gps_std_filename[7][STD_FILENAME_SIZE]={0};
	for(i=0;i<7;i++)
	{
        sprintf(gps_std_filename[i],"%s%04d%s",pre_filename,gps_week,post_filename[i]);
        checkfile(gps_std_filename[i],GPS);
	}
}

/**
 *      function    :   form the name of every month's check task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int yyyy,int mm}
        return      :   {void}
**/

void monthtask(int yyyy,int mm)
{
	char std_filename[STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="AAC";
	char post_filename[POST_FILENAME_SIZE]=".dcb.Z";
    sprintf(std_filename,"%s%04d%02d%s",pre_filename,yyyy,mm,post_filename);
    //standard file name:AACyyyymm.dcb.Z
    checkfile(std_filename,BD);
    checkfile(std_filename,GPS);
}

/**
 *      function    :   form the name of every month's check task
                        different form the monthtask() in checking time and name
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int yyyy,int mm}
        return      :   {void}
**/

void monthtask1(int yyyy,int mm)
{
	char std_filename[STD_FILENAME_SIZE]={0};
	char pre_filename[PRE_FILENAME_SIZE]="AAI";
	char post_filename[POST_FILENAME_SIZE]=".isa.Z";
    sprintf(std_filename,"%s%04d%02d%s",pre_filename,yyyy,mm,post_filename);
    checkfile(std_filename,BD);
    checkfile(std_filename,GPS);
    //standard file name:ACIyyyymm.isa.Z
}

/**
 *      function    :   be called every hour
                        accord to month, day, week, hour to call different functions
                        Then check file is upload in time
 *      para        :   {void}
        return      :   {void}
**/

void analysisCenterCheckTask()
{
    /*get current time then record the year,month,etc*/
    struct tm *t2;
    t2=gettime();
    //the year we get is ralated to 1900
    int year=t2->tm_year+1900;
    //the month is between 0~11
    int month=t2->tm_mon+1;
    //the day is between 1~31
    int day=t2->tm_mday;
    //weekday between 0~6 ,0:Sunday
    int wday=t2->tm_wday;
    //current hour
    int hour=t2->tm_hour;
    //BEIDOU week,the number of weeks since 2006.1.1
    int wwww = BTS_Time(year,month,day);

    /*
    6 hour task,happened at 2:00,8:00,14:00,20:00
    find out the file producted 2 hour ago has been uploadde
    */
    if(hour==2||hour==8||hour==14||hour==20)
        hour6task(wwww,wday,hour-2);

    /*
    everyday task,happened at 13:00 and 18:00
    find out the file producted 1 day ago has been uploadde
    */
    if(hour==13)
    {
        if(wday==0) daytask(wwww-1,6);
        else daytask(wwww,wday-1);
    }
    if(hour==18)
    {
        if(wday==0) daytask1(wwww-1,6);
        else daytask1(wwww,wday-1);
    }

    /*
    every week task,happened at Tuesday 00:00
    find out the file producted 2 weeks ago has been uploadde
    */
    if(wday==1&&hour==0)  weektask(wwww-2);

    /*
    every month task,happened at the first day of each month,different from files
    find out the file producted 1 month ago has been uploadde
    */
    if(day==1&&hour==18)
    {
        if(month==1) monthtask(year-1,12);
        else monthtask(year,month-1);
    }
    if(day==2&&hour==0)
    {
        if(month==1) monthtask1(year-1,12);
        else monthtask1(year,month-1);
    }

    /*
    hour task,happened every hour
    find out the file producted 1 hour ago has been uploaded
    we will check the task after 10 seconds.As so far, we adopt double "for" circle,h
    however, we do not make sure to delay 10 seconds.
    */
    //sleep(10);
    int b = 0, e = 0;
    for(b=0;b<20000;b++)
    for(e=0;e<20000;e++);

    if(hour==0)
    {
        if(wday==0) hourtask(wwww-1,6,23);
        else hourtask(wwww,wday-1,hour-1);
    }
    else hourtask(wwww,wday,hour-1);

}
