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

/********  宏定义  ********/
#define INOTIFY_EVENTS IN_CLOSE_WRITE|IN_DELETE     //events of monitor
#define INOTIFY_PATH "/home/jung/ftp/upload"        //path of monitor

/********  全局变量  ********/
struct TaskNode * uploadList;                       //head of the uploadlist
struct TaskNode * tail;                             //tail of the uploadlist.for insert


/**
 *      function    :   get current time
        return      :   {time_t}
**/
struct tm * gettime()
{
	time_t t;
	t=time(NULL);
	return localtime(&t);
}



char * getchartime()
{
	time_t t;
	t=time(NULL);
	return ctime(&t);
}


void write_log(TaskNode * p,char * a)
{
	struct tm *t;
	char * curtime;
	int k;
	t=gettime();
	curtime=getchartime();
	int year=t->tm_year+1900;
	int month=t->tm_mon+1;
	char name[5]=".txt";
	char logpath[12];
	sprintf(logpath,"%04d%02d%s",year,month,name);
	FILE *fp;
	fp=fopen(logpath,"a+");
	fprintf(fp,"%s\t%s\t%s\n",a,p->filename,curtime);
//	fprintf(fp,"%s\n",a);
	fclose(fp);
}

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
 *      function    :   insert node into the tail of the uploadlist
 *      para        :   {TaskNode *}   like "127.0.0.1"
**/
void insertlist(TaskNode * p0)
{
    p0->next=NULL;
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
    display();
}

/**
 *      function    :   search
 *      para        :   {char *}   like "ACC03957.sum.Z"
**/
void search(char * name)
{

	TaskNode *p1,*p2;
	if(uploadList==NULL)  //为空
	{
        printf("文件未创建\n");
        write_log(NULL,"文件未创建");
	}
    else
    {
        p1=uploadList;

        while(strcmp(name,p1->filename)!=0 && p1->next!=NULL)
        {
            p2=p1;
            p1=p1->next;
        }

        if(strcmp(name,p1->filename)==0)
        {
            if(p1->state==2)
            {
                if(p1==uploadList)
                if(p1==tail) {uploadList=NULL;tail=NULL;}
                else uploadList=p1->next;
                else p2->next=p1->next;
                write_log(p1,"按时完成");
                free(p1);
                printf("%s:文件上传成功\n",name);

            }
            else if(p1->state==1)
            {
               printf("%s:state=1文件正在上传\n",name);
               write_log(p1,"检查中上传");
            }
            else
            {
                printf("%s:state=0文件为上传\n",name);//可以继续调用ftp
                write_log(p1,"检查时失败");
            }

        }
        else printf("%s:文件未生成\n",name);//待定
        display();
    }

}

/**
 *      function    :   count the number of the week since 2006.1.1
 *      para        :   {time_t *}
        return      :   {int}
**/
int BTS_Time(struct tm * p)
{
return (mktime(p)-1136044800)/60/60/24/7;
}

/**
 *      function    :   count the number of the week since 2006.1.1
 *      para        :   {time_t *}
        return      :   {int}
**/
static void _inotify_event_handler(struct inotify_event *event, int eventNum)
{
    if(event->mask & IN_CLOSE_WRITE)
    {
        char command[50];
        FILE *pf;
        sprintf(command,"compress -f %s",event->name);
        if((pf=popen(command,"r"))==NULL)
        {
            perror("压缩失败");
        }
        pclose(pf);
    }
 	if(event->mask & IN_DELETE)
	{

		printf("IN_DELETE\n");
		printf("event->name: %s\n", event->name);
        char unix_z=[3]=".Z";
		TaskNode *p0;
		p0=(TaskNode *)malloc(sizeof(TaskNode));
		//strcpy(p0->filename,event->name);
		sprintf(p0->filename,"%s%s",event->name,unix_z);
		//printf("传入：%s\n",p0->filename);
		p0->state=0;//创建完
		write_log(p0,"文件创建");
		insertlist(p0);
	}
}

/**
 *      function    :   the main function of the monitor
**/

void analysisCenterMonitor()
{

	unsigned char buf[1024] = {0};
	struct inotify_event *event = {0};
	int fd = inotify_init();
	int wd = inotify_add_watch(fd,INOTIFY_PATH ,INOTIFY_EVENTS);    //add watch event


	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	while(1)
	{
		if (select(fd + 1, &fds, NULL, NULL, NULL) > 0)
		{
			int len, index = 0,indexTemp=0;
			while (((len = read(fd, &buf, sizeof(buf))) < 0) && (errno == EINTR));
			int eventnum = 0;
			while (indexTemp < len)
			{
				event = (struct inotify_event *)(buf + indexTemp);
				indexTemp += sizeof(struct inotify_event) + event->len;
				eventnum++;
			}
			printf("\nevent num = %d\n",eventnum);
			while (index < len)
			{
				event = (struct inotify_event *)(buf + index);
				_inotify_event_handler(event,--eventnum);
				printf("\n.........event num........... = %d\n",eventnum);
				index += sizeof(struct inotify_event) + event->len;
			}
			eventnum = 0;
		}
	}
	inotify_rm_watch(fd, wd);

}



/**
 *      function    :   count the number of the week since 2006.1.1
 *      para        :   {time_t *}
        return      :   {int}
**/
void hourtask(int w_4,int d,int hr)
{
	char a[20]; //保存文件名
	char abb[4]="ACI";
	char name[7]=".irt.Z";
    sprintf(a,"%s%04d%d_%02d%s",abb,w_4,d,hr,name);
    search(a);
}


void hour6task(int w_4,int d,int hr)
{
	char a[3][20]; //保存文件名
	char abb[4]="ACU";
	char name[3][7]={".sp3.Z",".erp.Z",".tro.Z"};
	int i=0;
	for(i=0;i<3;i++)
	{
		sprintf(a[i],"%s%04d%d_%02d%s",abb,w_4,d,hr,name[i]);
		search(a[i]);	//查找函数
	}
}

void daytask(int w_4,int d)
{
	char a[4][20]; //保存文件名
	char abb[4]="ACR";
	char name[4][7]={".sp3.Z",".clk.Z",".erp.Z",".ion.Z"};
	int i=0;
	for(i=0;i<4;i++)
	{
        sprintf(a[i],"%s%04d%d%s",abb,w_4,d,name[i]);
        search(a[i]);	//查找函数
	}
}

void weektask(int w_4)
{
	char a[7][20]; //保存文件名
	char abb[4]="AAC";
	char name[7][8]={"7.sp3.Z","7.clk.Z","7.snx.Z","7.erp.Z","7.tro.Z","7.ion.Z","7.sum.Z"};
	int i=0;
	for(i=0;i<7;i++)
	{
        sprintf(a[i],"%s%04d%s",abb,w_4,name[i]);
        search(a[i]);	//查找函数
	}
}

void monthtask(int year,int month)
{
	char a[20]; //保存文件名
	char abb[4]="AAC";
	char name[7]=".dcb.Z";
    sprintf(a,"%s%04d%02d%s",abb,year,month,name);
    search(a);
   //查找函数
}

void monthtask1(int year,int month)
{
	char a[20]; //保存文件名
	char abb[4]="AAI";
	char name[7]=".isa.Z";
    sprintf(a,"%s%04d%02d%s",abb,year,month,name);
    search(a);
   //查找函数
}
//定时一小时执行下面程序段
void analysisCenterCheckTask()
{

     struct tm *t2;
     t2=gettime();
     int year=t2->tm_year; //1900来的绝对时间
     int month=t2->tm_mon+1; // 0 到 11
     int day=t2->tm_mday;  //1 到 31
     int wday=t2->tm_wday;
     int hour=t2->tm_hour;
     int wwww = BTS_Time(t2);
     int min=t2->tm_min;

	 hourtask(wwww,wday,hour-1);
	 if(hour==2||hour==8||hour==14||hour==20)  hour6task(wwww,wday,hour-2);
	 if(hour==13)
	 {
		if(wday==0) daytask(wwww-1,6);
		else daytask(wwww,wday-1);
	 }
	 if(hour==18)
	 {
		if(wday==0) daytask(wwww-1,6);
		else daytask(wwww,wday-1);
	 }
	 if(wday==1&&hour==0)  weektask(wwww-2);
	 if(day==1&&hour==18)
	 {
		if(month==1) monthtask(year+1899,12);
		else monthtask(year+1900,month-1);
	 }
	 if(day==2&&hour==0)
	 {
		if(month==1) monthtask1(year+1899,12);
		else monthtask1(year+1900,month-1);
	 }

}



