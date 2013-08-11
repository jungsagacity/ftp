#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <string.h>

#include "download.h"


int    tmp=-1;

DownloadList downloadList;
extern FtpServer *fs;
extern DownInfo *downInfoList;
extern StationList sl;


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
        printf("%d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",p->id,p->source,p->timeType,p->fileType,p->stationList,p->downloadServer,p->dataCenterPath,p->localPath);
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
        free(q->stationList);
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

int readDownloadInfo(char * downloadInfoFile, DownInfo * downInfoList)
{
    FILE *fp;
    char delims[]="\t";
    char buf[BUF_SIZE];
    char * tmp = NULL;
    if((fp=fopen(downloadInfoFile,"r"))==NULL)
    {
#ifdef DEBUG
        printf("cannot open file\n");
#endif
        return 0;
    }
    printf("df\n");


    //read lines
    while(fgets(buf,BUF_SIZE,fp)!=NULL)
    {
        int i = 0;
        removespace(buf);
        //spilt the line with the "\t"
        printf("%s\n",buf);
        tmp=strtok(buf,delims);
        int id = atoi(tmp);

#ifdef DEBUG
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
#ifdef DEBUG
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
#ifdef DEBUG
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
#ifdef DEBUG
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
#ifdef DEBUG
                    printf("fileType:%s\n",dw->fileType);
#endif
                    break;

                }
                //stationList like:igu.sta
                case 3:
                {

                    int len=strlen(tmp)+1;
                    dw->stationList = (char *)malloc(len);
                    memset(dw->stationList,0, len);
                    strcpy(dw->stationList, tmp);
#ifdef DEBUG
                    printf("stationList:%s\n",dw->stationList);
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
#ifdef DEBUG
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
#ifdef DEBUG

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
#ifdef DEBUG
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

