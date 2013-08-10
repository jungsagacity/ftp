#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

#include "download.h"

int    tmp=-1;

extern FtpSever *fs;
DownloadList downloadList;
extern DownInfo *downInfoList;
extern StationList sl;


/*
int main()
{
    downInfoList= initDownInfolist();
    readtxt(downInfoList);    //displayDW(DI);


    downloadList=(DownloadNode*)malloc(sizeof(DownloadNode));
    memset(downloadList, 0, sizeof(DownloadNode));
    downloadList->next = NULL;
    time_module_control();

    test1:
            creat_list(2013,220,24,0,downInfoList);
            DownloadNode *p=downloadList->next;
            while(p!=NULL)
            {
                printf("%s\n%s\n%s\n\n",p->filename,p->remotePath,p->localPath);
                //printf("taskNum=%d\n%s\n%s\n",p->taskNum,p->state,p->server->ip);
                p=p->next;
            }

}
*/




/******************************************************************************************
*    function   :
*    para       :
*
*    return     :
*    history    :   {2013.7.27 yangyang} create;
*                   {2013.8.8 yangyang}  modify
******************************************************************************************/

void time_module_control()
{

    MYtime mt;
    MYtime *pmt = &mt;
    int test=-1;
    while(1)
    {
        get_currentime(pmt);    //get current time
        if(mt.minute==10||mt.minute==25||mt.minute==40||mt.minute==55)//the condition to check the file whether exist
        {
            if(test!=mt.minute)
            {
                test=mt.minute;

                if(mt.minute==25&&mt.hour==0)//at the begin of a day ,init the DownloadNode;
                {
                    DownloadNode *p=downloadList->next;
                    DownloadNode *q=p->next;
                    downloadList->next=NULL;
                    while(q!=NULL)
                    {
                        free(p);
                        p=q;
                        q=p->next;
                    }
                    free(p);
                }
                //mt store the check time and then get the time that the file begin to transport
                int year,day,hour,minute;
                minute=mt.minute-10;
                if(minute==0)
                {
                    if(mt.hour==0&&mt.day==1)
                    {
                        year=mt.year-1;
                        hour=24;     //when deal with the time 00:00 ,transfer to 4:00
                        if(year%4==0&&year%100!=0)
                            day=R_year;
                        else
                            day=P_year;
                    }
                    else if(mt.hour==0)
                    {
                        year=mt.year;
                        day=mt.day-1;
                        hour=24;
                    }
                    else
                    {
                        hour=mt.hour;
                        year=mt.year;
                        day=mt.day;
                    }
                }
                else
                {
                    year=mt.year;
                    day=mt.day;
                    hour=mt.hour;
                }
                printf("the file transport time is:%d %d %d:%d\n",year,day,hour,minute);
                if(minute%15==0)
                {
                    //creat the download list
                    creat_list(year,day,hour,minute,downInfoList);
                    #ifdef DEBUG
                    DownloadNode *p=downloadList->next;

                    while(p!=NULL)
                    {
                        printf("%s\n%s\n%s\n\n",p->filename,p->remotePath,p->localPath);
                        //printf("taskNum=%d\n%s\n%s\n",p->taskNum,p->state,p->server->ip);
                        p=p->next;
                    }
                    #endif

                }
            }
        }
    }
}


/******************************************************************************************
*    function   :
*    para       :
*
*    return     :
*    history    :   {2013.8.9 yangyang} frist create;
******************************************************************************************/

void creat_list(int year,int day,int hour,int minute,DownInfo *DI)
{
    //download include(daily,hourly,highrate)
    if(minute==0&&hour==24)
    {
        minute=(minute+60-15)%60;
        hour=(hour+24-1)%24;
        DownInfo *p=DI->next;
        while(p!=NULL)
        {
            //creat node
            DownloadNode *s;
            s=(DownloadNode*)malloc(sizeof(DownloadNode));
            memset(s, 0, sizeof(DownloadNode));
            add_Info(s,p,year,day,hour,minute);

            //insert the node at the head
            s->next=downloadList->next;
            downloadList->next=s;
            p=p->next;
        }
    }

    //download include(hourly,highrate)
    else if(minute==0)
    {
        minute=(minute+60-15)%60;
        hour=(hour+24-1)%24;
        DownInfo *p=DI->next;
        while(p!=NULL)
        {
            //creat node
            DownloadNode *s;
            s=(DownloadNode*)malloc(sizeof(DownloadNode));
            memset(s, 0, sizeof(DownloadNode));
            if(strcmp(p->timeType,Hourly)==0||strcmp(p->timeType,Highrate)==0)
            {
                add_Info(s,p,year,day,hour,minute);
                s->next=downloadList->next;
                downloadList->next=s;
            }

            //insert the node at the head

            p=p->next;
        }
    }

    //download include(highrate)
    else
    {
        minute=(minute+60-15)%60;
        DownInfo *p=DI->next;
        while(p!=NULL)
        {
            //creat node
            DownloadNode *s;
            s=(DownloadNode*)malloc(sizeof(DownloadNode));
            memset(s, 0, sizeof(DownloadNode));
            if(strcmp(p->timeType,Highrate)==0)
            {
                add_Info(s,p,year,day,hour,minute);
                s->next=downloadList->next;
                downloadList->next=s;
            }
            //insert the node at the head

            p=p->next;
        }
    }
}


/******************************************************************************************
*    function   :
*    para       :
*
*    return     :   {void}
*    history    :   {2013.8.10 yangyang} frist create
******************************************************************************************/

void add_Info(DownloadNode *s,DownInfo *p,int year,int day,int hour,int minute)
{

    //add filename
    s->filename=(char*)malloc(sizeof(char)*MAX_size);
    memset(s->filename,0,MAX_size);
    creat_filename(year,day,hour,minute,p->fileType,p->timeType,s->filename);

    //add filepath(server local)
    s->remotePath=replace_path(p->dataCenterPath,year,day,hour,minute,p->fileType);
    s->localPath=replace_path(p->localPath,year,day,hour,minute,p->fileType);



    //add ftpserver
    FtpSever *f=fs->next;
    while(strcmp(p->downloadServer,f->ip)!=0)
    {
        f=f->next;
    }
    s->server=f;

    //add  station list
    StationList *w=sl->next;
    while(strcmp(p->stateList,w->name)!=0)
    {
        w=w->next;
    }
    s->station=w->list;

    int i=0;
    int k=0;
    s->state=(char*)malloc(sizeof(char)*MAX_size);
    memset(s->state,0,MAX_size);

    while(s->station[i]!=NULL)
    {
        char *filepath1=(char*)malloc(sizeof(char)*MAX_size);
        memset(filepath1,0,sizeof(char)*MAX_size);
        strcpy(filepath1,s->localPath);
        strcat(filepath1,s->filename);
        char *check1=replace(filepath1,stationame,s->stations[i]);
#ifdef  DEBUG
        printf("check1=%s\n",check1);
#endif
        char *tmp1=(char*)malloc(sizeof(char)*MAX_size);
        memset(tmp1,0,sizeof(char)*MAX_size);
        strcpy(tmp1,s->filename);
        char *tmp2=replace(tmp1,success_extension,extensioname);

        char *filepath2=(char*)malloc(sizeof(char)*MAX_size);
        memset(filepath2,0,MAX_size);
        strcpy(filepath2,s->localPath);
        strcat(filepath2,tmp2);
        char *check2=replace(filepath2,stationame,s->stations[i]);
#ifdef  DEBUG
        printf("check2=%s\n",check2);
#endif

        //add state
        if(Search_file(check1)==DOWNLOAD_FILE_EXIST||Search_file(check2)==DOWNLOAD_FILE_EXIST)
        {
            s->state[i]=DOWNLOAD_FILE_EXIST;
        }
        else
        {
            s->state[i]==DOWNLOAD_FILE_NONEXIST;
            k++;
        }
        i++
    }

    //add  taskNum
    s->taskNum=k;


}



/******************************************************************************************
*    function   :
*    para       :
*
*    return     :
*    history    :   {2013.8.9 yangyang} frist create;
******************************************************************************************/

char *replace(char *str1,char *str2,char *str3)
{
    char *p;
    p=strstr(str1,str2);
    int k=p-str1+1;
    char *q;
    q=(char*)malloc(sizeof(char)*MAX_size);
    memset(q,0,MAX_size);
    strncpy(q,str1,k-1);
    strcat(q,str3);
    strcat(q,p+strlen(str2));
    return q;
}


/******************************************************************************************
*    function   :
*    para       :
*
*    return     :
*    history    :   {2013.8.9 yangyang} frist create;
******************************************************************************************/

char *replace_path(char *rpath,int year,int day,int hour,int minute,char *type)
{
    char *p;
    //replace year
    char *yy1;
    p=strstr(rpath,yyyy);
    if(p!=NULL)
    {
        char *y;
        y=(char*)malloc(sizeof(char)*MAX_size);
        memset(y,0,MAX_size);
        sprintf(y,"/%04d/",year);
        yy1=replace(rpath,yyyy,y);
        free(y);
    }
    //replace day
    char *dd1;
    p=strstr(rpath,ddd);
    if(p!=NULL)
    {
        char *d;
        d=(char*)malloc(sizeof(char)*MAX_size);
        memset(d,0,MAX_size);
        sprintf(d,"/%03d/",day);
        dd1=replace(yy1,ddd,d);
        free(d);
    }
    char *ff1,*hh1,*mm1;
    p=strstr(rpath,yyf);
    if(p!=NULL)
    {
        char *f;
        f=(char*)malloc(sizeof(char)*MAX_size);
        memset(f,0,MAX_size);
        sprintf(f,"/%02d%c/",year%100,type[0]+32);
        ff1=replace(dd1,yyf,f);
        free(f);
        return ff1;
    }
    else
    {
        char *q;
        q=strstr(rpath,hh);
        if(q!=NULL)
        {
            char *h;
            h=(char*)malloc(sizeof(char)*MAX_size);
            memset(h,0,MAX_size);
            sprintf(h,"/%02d/",hour);
            hh1=replace(dd1,hh,h);
            free(h);

            char *s;
            s=strstr(rpath,mm);
            if(s!=NULL)
            {
                char *m;
                m=(char*)malloc(sizeof(char)*MAX_size);
                memset(m,0,MAX_size);
                sprintf(m,"/%02d/",minute);
                mm1=replace(hh1,mm,m);
                free(m);
                return mm1;
            }
            else
                return hh1;
        }
        else
            return dd1;

    }

}


/******************************************************************************************
*    function   :   creat the file name
*    para       :   {int year,int day,int hour,int minute,int type,char *highrate_filename}
*                      *highrate_filename  store the filename,type is the type of the file
*
*    return     :   {void}
*    history    :   {2013.7.23 yangyang} frist create;
******************************************************************************************/

void creat_filename(int year,int day,int hour,int minute,char *type,char *timetype,char filename[])
{
    if(strcmp(timetype,Daily)==0)
    {
        sprintf(filename,"%s%03d0.%02d%c.Z",stationame,day,year%100,type[0]+32);
    }
    else if(strcmp(timetype,Hourly)==0)
    {
        sprintf(filename,"%s%03d%c.%02d%c.Z",stationame,day,hour+97,year%100,type[0]+32);
    }
    else if(strcmp(timetype,Highrate)==0)
    {
        sprintf(filename,"%s%03d%c%02d.%02d%c.Z",stationame,day,hour+97,minute,year%100,type[0]+32);
    }
    //printf("%s\n",filename);
}


/******************************************************************************************
*    function   :   get the current data time and transfer two the user_defined struct
*    para       :   {MYtime *mt}  MYtime is the user_defined struct of time
*
*    return     :   {void}
*    history    :   {2013.7.25 yangyang} frist create;
******************************************************************************************/

void get_currentime(MYtime *mt)
{
    struct tm *currentime;
    time_t t;
    time(&t);               //get the millsecond
    currentime=localtime(&t);//store in the struct currentime
    mt->year=currentime->tm_year+1900;
    mt->month=currentime->tm_mon+1;
    mt->day=transfer(mt->year,mt->month,currentime->tm_mday);
    mt->hour=currentime->tm_hour;
    mt->minute=currentime->tm_min;
    mt->second=currentime->tm_sec;

    if(tmp!=mt->minute)
    {
        printf ( "Current date/time: %s", asctime (currentime));
        tmp=mt->minute;
    }
}


/**********************************************************************************
*    function   :   chang the data(month&&day) to the day of the year from 1 to 365
*    para       :   {int year,int month,int day}(like 2012.12.31->sum=366)
*
*    return     :   int sum;
*    history    :   {2013.7.20 yangyang} frist create;
***********************************************************************************/

int transfer(int year,int month,int day)
{
    int a[12]= {31,28,31,30,31,30,31,31,30,31,30,31};  //Days of every month
    int i,sum=0;
    for(i=0; i<month-1; i++)
    {
        sum+=a[i];
    }
    sum+=day;
    if(year%4==0&&year%100!=0&&month>2) sum++;
    return sum;
}


/************************************************************************************
*    function   :   check the file exist or not
*    para       :   {char *filename} the file name include the path(like F:\tmp\1.txt
*
*    return     :    int (DOWNLOAD_FILE_EXIST mean exist &&DOWNLOAD_FILE_NONEXIST not)
*    history    :   {2013.7.22 yangyang} frist create;
*************************************************************************************/

char Search_file(char filename[])
{
    int k=0;
    k=access(filename,0);
    if( k == 0)
        return DOWNLOAD_FILE_EXIST;
    else
        return DOWNLOAD_FILE_NONEXIST;
}









/****************************************
 *      function    :   display the list
 *      para        :   DownInfo * head
        return      :   void
****************************************/

void displayDW(DownInfo * head)
{
    DownInfo * p;
    p=head->next;
    while(p!=NULL)
    {
        printf("%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",p->id,p->source,p->timeType,p->fileType,p->stateList,p->downloadServer,p->dataCenterPath,p->localPath);
        p=p->next;
    }
}


/**
 *      function    :   Initialize list
 *      para        :   void
        return      :   {DownInfo * }
                        head of the list
**/


DownInfo * initDownInfolist()
{
    DownInfo * downInfoList = (DownInfo*)malloc(sizeof(DownInfo));
    downInfoList -> next = NULL;
    return downInfoList;
}

/**
 *      function    :   insert into the list
 *      para        :   {DownInfo * downInfoList,DownInfo * down}
                        DownInfo * downInfoList:the head node of the list
                        DownInfo * down        :the node which is going to be deleted
        return      :   {DownInfo * }
**/


DownInfo * addDownInfo(DownInfo * downInfoList,DownInfo * down)
{
    down->next = downInfoList->next;
    downInfoList->next = down;
}

/**
 *      function    :   delete the node ,free the memory
 *      para        :   {DownInfo * downInfoList,DownInfo * down}
                        DownInfo * downInfoList:the head node of the list
                        DownInfo * down        :the new node
        return      :   {void }
**/


void  delDownInfo(DownInfo * downInfoList,DownInfo * down)
{
    DownInfo * p =downInfoList,*q = downInfoList->next;
    while(q != down && q!=NULL)
    {
        p=q;
        q=q->next;
    }
    if(q==NULL)
    {
#ifdef DEBUG_1
        printf("The node is not exist\n");
#endif
    }
    else
    {
        p->next=q->next;
        free(q->localPath);
        free(q->dataCenterPath);
        free(q->downloadServer);
        free(q->stateList);
        free(q->fileType);
        free(q->timeType);
        free(q->source);
        free(q);
    }

}

/**
 *      function    :   remove the space in the string
 *      para        :   {char  * str}
        return      :   {void }
**/

void removespace(char  * str)
{
    if(str==NULL)
    {
        char * p=str;
        int i=0;
        while((*p)!=0)
        {
            if((*p)!=' ')
            {
                str[i++]=*p;
            }
            p++;
        }
        str[i]=0;
    }
}

/**
 *      function    :   main function of reading the download information file
 *      para        :   {DownInfo * downInfoList}
                        the head node of the list
        return      :   {int}   0:open file failed
                                1:success
**/

int readDownloadInfo(DownInfo * downInfoList)
{
    FILE *fp;
    char delims[]="\t";
    char buf[BUF_SIZE];
    char * tmp = NULL;
    if((fp=fopen(DOWN_INFO_PATH,"r"))==NULL)
    {
#ifdef DEBUG_1
        printf("cannot open file\n");
#endif
        return 0;
    }

    //read lines
    while(fgets(buf,BUF_SIZE,fp)!=NULL)
    {
        int i = 0;
        removespace(buf);
        //spilt the line with the "\t"
        tmp=strtok(buf,delims);
        int id = atoi(tmp);

#ifdef DEBUG_1
        printf("id:%d\n",id);
#endif

        //not the notes
        if(strstr(tmp,"#")==0)
        {
            //create the new node
            DownInfo * dw;
            dw = (DownInfo *)malloc(sizeof(DownInfo));

            while(tmp!=NULL)
            {
#ifdef DEBUG_1
                printf("tmp%d:%s\n",i,tmp);
#endif

                tmp = strtok(NULL,delims);

                dw->id=id;


                switch(i)
                {
                    //source like:GPS+GLONASS
                case 0:
                {
                    int len=strlen(tmp)+1;
                    dw->source = (char *)malloc(len);
                    memset(dw->source,0, len);
                    strcpy(dw->source, tmp);
#ifdef DEBUG_1
                    printf("source:%s\n",dw->source);
#endif
                    break;

                }
                //timeType like;hourly/daily
                case 1:
                {
                    int len=strlen(tmp)+1;
                    dw->timeType = (char *)malloc(len);
                    memset(dw->timeType,0, len);
                    strcpy(dw->timeType, tmp);
#ifdef DEBUG_1
                    printf("timeType:%s\n",dw->timeType);
#endif
                    break;

                }
                //fileType like: D/N/G
                case 2:
                {

                    int len=strlen(tmp)+1;
                    dw->fileType = (char *)malloc(len);
                    memset(dw->fileType,0, len);
                    strcpy(dw->fileType, tmp);
#ifdef DEBUG_1
                    printf("fileType:%s\n",dw->fileType);
#endif
                    break;

                }
                //stateList like:igu.sta
                case 3:
                {

                    int len=strlen(tmp)+1;
                    dw->stateList = (char *)malloc(len);
                    memset(dw->stateList,0, len);
                    strcpy(dw->stateList, tmp);
#ifdef DEBUG_1
                    printf("stateList:%s\n",dw->stateList);
#endif
                    break;

                }
                //downloadServer like:icddis.gsfc.nasa.gov
                case 4:
                {
                    int len=strlen(tmp)+1;
                    dw->downloadServer = (char *)malloc(len);
                    memset(dw->downloadServer,0, len);
                    strcpy(dw->downloadServer, tmp);
#ifdef DEBUG_1
                    printf("dw->downloadServer:%s\n",dw->downloadServer);
#endif
                    break;
                }
                //dataCenterPath like:pub/gps/data/hourly/yyyy/ddd/hh/
                case 5:
                {
                    int len=strlen(tmp)+1;
                    dw->dataCenterPath = (char *)malloc(len);
                    memset(dw->dataCenterPath,0, len);
                    strcpy(dw->dataCenterPath, tmp);
#ifdef DEBUG_1

                    printf("dw->dataCenterPath:%s\n",dw->dataCenterPath);
#endif
                    break;
                }
                //localPath  like:GNSS/yyyy/ddd/hourly/hh/
                case 7:
                {
                    int len=strlen(tmp)+1;
                    dw->localPath = (char *)malloc(len);
                    memset(dw->localPath,0, len);
                    strcpy(dw->localPath, tmp);
#ifdef DEBUG_1
                    printf("localPath:%s\n",dw->localPath);
#endif
                    break;

                }
                }
                i++;

            }
            addDownInfo(downInfoList,dw);
        }
    }
    fclose(fp);
    return 1;
}

