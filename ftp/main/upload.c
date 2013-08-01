#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/inotify.h>
#include <malloc.h>
#include <time.h>

#include "upload.h"

//报错错误以下
//!!!!static void _inotify_event_handler函数减少一个参数，main.c修改
//!!!!PRODUCT_PATH  未声明
//#define PRODUCT_PATH "/src"

/********  GLOBAL VARIBALES  ********/
struct TaskNode * uploadList;
//head of the uploadlist
struct TaskNode * tail;
//tail of the uploadlist for insert


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
    TaskNode *p;
    p=uploadList;
    while(p!=NULL)
    {
        printf("链表：%s\t%d\n",p->filename,p->state);
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
 *      function    :   insert node into the tail of the uploadlist
 *      para        :   {TaskNode *}
        return      :   void
**/

void insertlist(TaskNode * p0)
{
    p0->next=NULL;
    //insert into the end of the list
    if(uploadList==NULL)
    {
        uploadList=p0;
        tail=p0;
    }
    else
    {
        tail->next=p0;
        tail=p0;
    }
    #ifdef DEBUG
    display();
    #endif
}


/**
 *      function    :   search the list according to the name to check the file's state
 *      para        :   {char *}   like "ACC03957.sum.Z"
        return      :   void
**/

void search(char * name)
{

	TaskNode *p1,*p2,*p3,*p4;

	/*
	the list is empty which obvious means the file is not upload
	create a new node record the filename and state=4
	to inform the notify center
	*/
	if(uploadList==NULL)
	{
        #ifdef DEBUG
        printf("文件未创建\n");
        log_checktask(name,"文件未创建");
        #endif
        //create a new node ,the insert into list
        TaskNode *p0;
		p0=(TaskNode *)malloc(sizeof(TaskNode));
		strcpy(p0->filename,name);
		//state=UPLOAD_FILE_UPLOAD_LATE ,  not upload in time
 		p0->state=UPLOAD_FILE_UPLOAD_LATE;
        insertlist(p0);

	}

	/*
	the list is not empty
	go through the list to find the file accord to the name
	change the related state
	*/
    else
    {
        p1=uploadList;

        //go through the list until find the node or at the end
        while(strcmp(name,p1->filename)!=0 && p1->next!=NULL)
        {
            p2=p1;
            p1=p1->next;
        }

        //find the node you are searching for,change the state
        if(strcmp(name,p1->filename)==0)
        {
            /*
            state=UPLOAD_FILE_UPLOAD_SUCCESS means uploaded success
            change the state=UPLOAD_FILE_UPLOAD_INTIME,in time
            */
            if(p1->state==UPLOAD_FILE_UPLOAD_SUCCESS)
            {
                p1->state=UPLOAD_FILE_UPLOAD_INTIME;
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
                p1->state=UPLOAD_FILE_UPLOAD_LATE;
                #ifdef DEBUG
                printf("%s:state=0文件为上传\n",name);//可以继续调用ftp
                log_checktask(name,"文件未上传");
                #endif
            }
            else if(p1->state==UPLOAD_FILE_UPLOAD_FAILED)
            {
                p1->state=UPLOAD_FILE_UPLOAD_LATE;
                #ifdef DEBUG
                printf("%s:state=3文件上传失败\n",name);//可以继续调用ftp
                log_checktask(name,"文件上传失败");
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

        }
        /*
        go through the list can't find the node
        means file is not producted,not in time
        create a new node record the filename and state=4
        to inform the notify center
        */
        else
        {
            //create a new node ,insert into list
            TaskNode *p0;
            p0=(TaskNode *)malloc(sizeof(TaskNode));
            strcpy(p0->filename,name);
            p0->state=UPLOAD_FILE_UPLOAD_LATE;//未及时上传 state=4
            insertlist(p0);

            #ifdef DEBUG
            printf("%s:文件未生成\n",name);//待定
            display();
            #endif
        }
    }
    /*
        go through the list to find the node whose state=5
        which means the notify center has checked it
        then delete it
    */
    p3=uploadList;
    //go through the list until at the end
    while(p3!=NULL)
    {
        //state=UPLOAD_FILE_DELETABLE means the notify center has taken act,then delete the node
        if(p3->state==UPLOAD_FILE_DELETABLE)
        {
            //if the target node is the first
            if(p3==uploadList)
            {
                //at the same time the target node is the last
                if(p3==tail)
                {
                    uploadList=NULL;
                    tail=NULL;
                }
                else
                    uploadList=p3->next;
            }
            else
            {
                //if the target node is at the last
                if(p3==tail)
                {
                    tail=p4;
                    p4->next=NULL;

                }
                else
                    p4->next=p3->next;
            }
            //free the space of the target node
            free(p3);
            #ifdef DEBUG
            log_checktask(name,"文件上传成功");
            printf("%s:文件上传成功\n",name);
            pritnf("%s:节点删除成功\n",name);
            #endif
        }
        //get the next node
        p4=p3;
        p3=p3->next;

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

    /*File was closed */
    if(event->mask & IN_CLOSE_WRITE)
    {
        #ifdef DEBUG
		printf("IN_CLOSE_WRITE\n");
		printf("event->name: %s\n", event->name);
		#endif
        char command[COMMAND_SIZE];
        FILE *pf;
        //get the command to compress the file with unix.z
        sprintf(command,"compress -f %s%s",PRODUCT_CENTER_PATH_PREFIX,event->name);
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

    /* File was deleted */
 	if(event->mask & IN_DELETE)
	{
        #ifdef DEBUG
		printf("IN_DELETE\n");
		printf("event->name: %s\n", event->name);
		#endif
		TaskNode *p0;
		p0=(TaskNode *)malloc(sizeof(TaskNode));
		//the event name is without suffix ".Z"
		sprintf(p0->filename,"%s%s",event->name,UNIX_Z);
		//set the state=0,the file is producted,waiting upload
		p0->state=0;
        #ifdef DEBUG
        printf("传入：%s\n",p0->filename);
		log_checktask(p0->filename,"文件创建");
        #endif
        //create a new node
		insertlist(p0);
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
    //strcut and variables to store the monitor events
	unsigned char buf[BUF_MAX] = {0};
	struct inotify_event *event = {0};
	int fd = inotify_init();
	/*
	set monitor path with the PRODUCT_CENTER_PATH_PREFIX
	set the monitor condition with INOTIFY_EVENTS
	*/
	int wd = inotify_add_watch(fd,PRODUCT_CENTER_PATH_PREFIX ,INOTIFY_EVENTS);
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
	inotify_rm_watch(fd, wd);
}



/**
 *      function    :   form the name of every hour's checking task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d,int hr}
        return      :   {void}
**/

void hourtask(int wwww,int d,int hr)
{
	char std_filename[STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="ACI";
	char post_filename[POST_FILENAME_SIZE]=".irt.Z";
    sprintf(std_filename,"%s%04d%d_%02d%s",pre_filename,wwww,d,hr,post_filename);
    //standard file name:ACIwwwwd_HR.irt.Z
    search(std_filename);
}

/**
 *      function    :   form the name of every 6 hour's checking task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww,int d,int hr}
        return      :   {void}
**/

void hour6task(int wwww,int d,int hr)
{
	char std_filename[3][STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="ACU";
	char post_filename[3][POST_FILENAME_SIZE]={".sp3.Z",".erp.Z",".tro.Z"};
	int i=0;
	for(i=0;i<3;i++)
	{
		sprintf(std_filename[i],"%s%04d%d_%02d%s",pre_filename,wwww,d,hr,post_filename[i]);
		//standard file name:ACUwwwwd_HR.sp3.Z,ACUwwwwd_HR.erp.Z,ACUwwwwd_HR.tro.Z
		search(std_filename[i]);
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
	char std_filename[4][STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="ACR";
	char post_filename[3][POST_FILENAME_SIZE]={".sp3.Z",".clk.Z",".erp.Z"};
	int i=0;
	for(i=0;i<3;i++)
	{
        sprintf(std_filename[i],"%s%04d%d%s",pre_filename,wwww,d,post_filename[i]);
        //standard file name:ACUwwwwd.sp3.Z,ACUwwwwd.clk.Z,ACUwwwwd.erp.Z
        search(std_filename[i]);
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
	char std_filename[STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="ACR";
	char post_filename[POST_FILENAME_SIZE]=".ion.Z";
    sprintf(std_filename,"%s%04d%d%s",pre_filename,wwww,d,post_filename);
    //standard file name:ACRwwwwd.ion.Z
    search(std_filename);
}

/**
 *      function    :   form the name of every week's check task
                        check the uploadlist to find out the file is upload in time
 *      para        :   {int wwww}
        return      :   {void}
**/

void weektask(int wwww)
{
	char std_filename[7][STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="AAC";
	char post_filename[7][POST_FILENAME_SIZE]={"7.sp3.Z","7.clk.Z","7.snx.Z","7.erp.Z","7.tro.Z","7.ion.Z","7.sum.Z"};
	int i=0;
	for(i=0;i<7;i++)
	{
        sprintf(std_filename[i],"%s%04d%s",pre_filename,wwww,post_filename[i]);
        /*
        standard file name:
        AACwwww7.sp3.Z,AACwwww7.clk.Z,AACwwww7.snx.Z
        AACwwww7.erp.Z,AACwwww7.tro.Z,AACwwww7.ion.Z,AACwwww7.sum.Z
        */
        search(std_filename[i]);
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
	char std_filename[STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="AAC";
	char post_filename[POST_FILENAME_SIZE]=".dcb.Z";
    sprintf(std_filename,"%s%04d%02d%s",pre_filename,yyyy,mm,post_filename);
    //standard file name:AACyyyymm.dcb.Z
    search(std_filename);
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
	char std_filename[STD_FILENAME_SIZE];
	char pre_filename[PRE_FILENAME_SIZE]="AAI";
	char post_filename[POST_FILENAME_SIZE]=".isa.Z";
    sprintf(std_filename,"%s%04d%02d%s",pre_filename,yyyy,mm,post_filename);
    search(std_filename);
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
    hour task,happened every hour
    find out the file producted 1 hour ago has been uploadde
    */
    hourtask(wwww,wday,hour-1);

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
        if(wday==0) daytask(wwww-1,6);
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
}