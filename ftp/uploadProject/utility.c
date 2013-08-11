#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdarg.h>

#include "ftp.h"

/******************************************************************************************
*                                   global variables                                      *
******************************************************************************************/
FtpServer   *fs;//store all ftp server info in system.ini file
UploadPath  *uploadPath;//stor upload path info in sysytem.ini file



/************************************************************************************************
*    function   :   clear all blank space char
*    para       :   {char *str} the target string
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
*************************************************************************************************/
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

/************************************************************************************************
*    function   :   delay some time
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
*************************************************************************************************/
void delay()
{
    int b=0,e=0;
    for(b=0;b<2000;b++)
    for(e=0;e<2000;e++);
}


/*************************************************************************************************
*    function   :   write log to file
*    para       :   {char *format} content format;
*
*    return     :   {int } 0: ok; -1:error.
*
*    history    :   {2013.8.5 wujun} frist create;
**************************************************************************************************/
int writeLog(char *format,...)
{
   	if( format == NULL)
   	{
        #ifdef DEBUG
        printf("Fun(writeLog): parameter is Null.\n");
        #endif
        return -1;
   	}

   	struct tm 	*st;
	time_t 		t;

    t = time(NULL);
    st = localtime(&t);//get current datetime

    int day = st->tm_mday + 1;
    int month = st->tm_mon + 1;
    int year = st->tm_year + 1990;

    char logFile[100] ;
    memset(logFile, 0, 100);

    sprintf(logFile, "log/%04d%02d%02d.log", year, month, day);
    dirIsExist("log/");
    FILE *dwLogFp;
    if( (dwLogFp=fopen( logFile ,"at") ) == NULL )
    {
        dwLogFp = fopen( logFile ,"wt");
        if( dwLogFp == NULL )
        {
            #ifdef DEBUG
            printf("Fun(writeLog):create file %s error.\n ",logFile);
            #endif
            return -1;
        }
    }

    va_list arg_ptr;
    va_start(arg_ptr, format);
    vfprintf(dwLogFp, format, arg_ptr);
    va_end(arg_ptr);

    fclose(dwLogFp);

}



/**************************************************************************************************
*    function   :   load system.ini file
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int loadSystem(char *conf)
{
    int k=0;
    FILE *fp = fopen(conf, "r");
    if(fp == NULL)
    {
        #ifdef DEBUG
        printf("Fun(loadSystem):%s open error.\n",conf);
        #endif
        return -1;
    }

    FtpServer  *fsNode;

    uploadPath = (UploadPath*)malloc(sizeof(UploadPath));
    memset(uploadPath, 0, sizeof(UploadPath));

    char buff[1000];
    memset(buff, 0, 1000);
    while(fgets(buff, 1000, fp) != NULL)
    {
        if((buff[0] == '#') || (buff[0] == '\n') )
        {
            continue;
        }

        clearSpace(buff);

        int i, j;
        i = j =0;
        while(buff[i++] != ':');
        char *name;
        name = (char *)malloc(i);
        if(name == NULL)
        {
            #ifdef DEBUG
            printf("Fun(loadSystem): malloc \"name\" error.\n");
            #endif
            return -1;
        }

        memset(name, 0, i);
        strncpy(name, buff, i-1);
        j = i;

        int parNum = 0;
        if( !strcmp(name,"productCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
            if(fsNode == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc product center \"fsNode\" error.\n");
                #endif
                return -1;
            }
            memset(fsNode, 0, sizeof(FtpServer));

            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                switch( ++parNum )
                {
                    case 1://ftp server ip
                    {
                        fsNode->ip = (char *)malloc(i-j);
                        if( fsNode->ip == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->ip\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->ip, 0, i-j);
                        strncpy(fsNode->ip,buff+j, i-j-1);
                        j = i;
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
                        j = i;
                        break;
                    }
                    case 3://username
                    {
                        fsNode->username = (char *)malloc(i-j);
                        if( fsNode->username == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->username\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->username, 0, i-j);
                        strncpy(fsNode->username,buff+j, i-j-1);
                        j = i;

                        if( buff[i-1] == '\n')
                        {
                            fsNode->passwd = NULL;
                        }
                        break;
                    }
                    case 4://passwd
                    {
                        fsNode->passwd = (char *)malloc(i-j);
                        if( fsNode->passwd == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc \"fsNode->passwd\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->passwd, 0, i-j);
                        strncpy(fsNode->passwd,buff+j, i-j-1);
                        break;
                    }
                    default:break;
                }
            }

            //make sure the product cener fpt info placed in the first node.
            fs->next = fsNode;
            fsNode ->next = NULL;
            fsNode = NULL;
        }

        if( !strcmp(name,"dataCenterFtp"))
        {
            fsNode = (FtpServer*)malloc(sizeof(FtpServer));
            if( fsNode == NULL )
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc dataCenter \"fsNode\" error.\n");
                #endif
                return -1;
            }

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
                        if( fsNode->ip == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->ip\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->ip, 0, i-j);
                        strncpy(fsNode->ip,buff+j, i-j-1);
                        j = i;
                        break;
                    }
                    case 2://port
                    {
                        char port[10] = {0};
                        strncpy(port,buff+j, i-j-1);
                        fsNode->port = atoi(port);
                        j = i;
                        break;
                    }
                    case 3://username
                    {
                        fsNode->username = (char *)malloc(i-j);
                        if( fsNode->username == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->username\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->username, 0, i-j);
                        strncpy(fsNode->username,buff+j, i-j-1);
                        j = i;

                        if( buff[i-1] == '\n')
                        {
                            fsNode->passwd = NULL;
                        }
                        break;
                    }
                    case 4://passwd
                    {
                        fsNode->passwd = (char *)malloc(i-j);
                        if( fsNode->passwd == NULL )
                        {
                            #ifdef DEBUG
                            printf("Fun(loadSystem): malloc dataCenter \"fsNode->passwd\" error.\n");
                            #endif
                            return -1;
                        }
                        memset(fsNode->passwd, 0, i-j);
                        strncpy(fsNode->passwd,buff+j, i-j-1);

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
                FtpServer  *tmp = fs->next->next;
                fsNode ->next = tmp;
                fs->next->next = fsNode;
            }

            fsNode = NULL;
        }

        if( !strcmp(name,"stations"))
        {
            while(buff[i-1] !='\n')
            {
                while(buff[i++] != '\t' && buff[i-1] != '\n');
                char *statesionFileName;
                statesionFileName = (char *)malloc(i-j);
                if( statesionFileName == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"statesionFileName\" error.\n");
                    #endif
                    return -1;
                }
                memset(statesionFileName, 0, i-j);
                strncpy(statesionFileName,buff+j, i-j-1);

                j = i;

                StationNode *slNode,*temp;
                slNode = (StationNode *)malloc(sizeof(StationNode));
                if( slNode == NULL )
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"slNode\" error.\n");
                    #endif
                    return -1;
                }
                memset(slNode, 0, sizeof(StationNode));

                FILE *fp = fopen(statesionFileName,"r");
                if(fp == NULL)
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): %s does not exist.\n",statesionFileName);
                    #endif
                    return -1;
                }

                char buf[7];
                char (*station)[5];

                while(fgets(buf,7,fp))
                {
                    k++;
                }

                station = (char **)malloc(k*5);
                if(station == NULL)
                {
                    #ifdef DEBUG
                    printf("Fun(loadSystem): malloc \"station\" error.\n");
                    #endif
                    return -1;
                }
                memset(station,0,k*5);

                fseek(fp, 0,SEEK_SET);
                int t = 0;
                 while(fgets(buf,7,fp))
                {
                    if(t<k)
                    {
                        strncpy(station[t++], buf, 4);
                    }
                    memset(buf, 0, 7);

                }

                slNode ->name = statesionFileName;
                slNode ->station = station;
                slNode ->stationNum = k;
                temp = sl->next;
                sl->next = slNode;
                slNode ->next = temp;

                fclose(fp);
                delay();
                k = 0;
            }
        }

        if( !strcmp(name,"tempDownloadFileSuffix"))
        {
            while(buff[i++] !='\n');
            tempDownloadFileSuffix = (char *)malloc(i-j);
            if(tempDownloadFileSuffix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"tempDownloadFileSuffix\" error.\n");
                #endif
                return -1;
            }
            memset(tempDownloadFileSuffix, 0, i-j);
            strncpy( tempDownloadFileSuffix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"tempUploadFileSuffix"))
        {
            while(buff[i++] !='\n');
            tempUploadFileSuffix = (char *)malloc(i-j);
            if(tempUploadFileSuffix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"tempUploadFileSuffix\" error.\n");
                #endif
                return -1;
            }
            memset(tempUploadFileSuffix, 0, i-j);
            strncpy( tempUploadFileSuffix, buff+j, i-j-1);
            j = i;
        }

        if( !strcmp(name,"maxDownloadTaskNum"))
        {
            while(buff[i++] !='\n');
            char *num = (char *)malloc(i-j);
            if(num == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"num\" error.\n");
                #endif
                return -1;
            }
            memset(num, 0, i-j);
            strncpy( num, buff+j, i-j-1);
            j = i;
            maxDownloadTaskNum = atoi(num);

        }

        if( !strcmp(name,(const char *)"BDLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefix = (char *)malloc(i-j);
            if(uploadPath->BDLocalPathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDLocalPathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefix, buff+j, i-j-1);
            j = i;
        }

        if( !strcmp(name,"GNSSLocalPathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefix = (char *)malloc(i-j);
            if(uploadPath->GNSSLocalPathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSLocalPathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSLocalPathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"BDLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDLocalPathPrefixBak = (char *)malloc(i-j);
            if(uploadPath->BDLocalPathPrefixBak == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDLocalPathPrefixBak\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->BDLocalPathPrefixBak, buff+j, i-j-1);
            j = i;

        }


        if( !strcmp(name,"GNSSLocalPathPrefixBak"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSLocalPathPrefixBak = (char *)malloc(i-j);
            if(uploadPath->GNSSLocalPathPrefixBak == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSLocalPathPrefixBak\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSLocalPathPrefixBak, 0, i-j);
            strncpy( uploadPath->GNSSLocalPathPrefixBak, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"BDRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->BDRemotePathPrefix = (char *)malloc(i-j);
            if(uploadPath->BDRemotePathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->BDRemotePathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->BDRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->BDRemotePathPrefix, buff+j, i-j-1);
            j = i;

        }


        if( !strcmp(name,"GNSSRemotePathPrefix"))
        {
            while(buff[i++] !='\n');
            uploadPath->GNSSRemotePathPrefix = (char *)malloc(i-j);
            if(uploadPath->GNSSRemotePathPrefix == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"uploadPath->GNSSRemotePathPrefix\" error.\n");
                #endif
                return -1;
            }
            memset(uploadPath->GNSSRemotePathPrefix, 0, i-j);
            strncpy( uploadPath->GNSSRemotePathPrefix, buff+j, i-j-1);
            j = i;

        }

        if( !strcmp(name,"downloadInfoFile"))
        {

            while(buff[i++] !='\n');
            downloadInfoFile = (char *)malloc(i-j);
            if(downloadInfoFile == NULL)
            {
                #ifdef DEBUG
                printf("Fun(loadSystem): malloc \"downloadInfoFile\" error.\n");
                #endif
                return -1;
            }
            memset(downloadInfoFile, 0, i-j);
            strncpy( downloadInfoFile, buff+j, i-j-1);
            j = i;

        }
    }

    fclose(fp);
    delay();
}

/**************************************************************************************************
*    function   :   merge staion lists
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int mergeStationList(char * name, char (*stationFileNames)[MAX_STATION_FILE_NAME_SIZE], int arrayLines)
{
    if( (name == NULL)  || (stationFileNames == NULL))
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): parameter is NULL.\n ");
        #endif
        return -1;
    }

    int i;
    int j = 0;
    int totalStations = 0;
    StationNode * p = sl->next;

    for(i=0;i<arrayLines;i++)
    {
        p = sl->next;
        while( p != NULL)
        {
            if( (p->name == NULL) || (stationFileNames[i] == NULL))
            {
                #ifdef DEBUG
                printf("Fun(mergeStationList): p->name or stationFileNames[i] is NULL.\n ");
                #endif
                return -1;
            }

            if(!strcmp(p->name, stationFileNames[i]))
            {
               totalStations += p->stationNum;
            }

            p = p->next;
        }
    }

    char (*newStaionArray)[5];
    newStaionArray = (char **)malloc(totalStations*5);
    if( newStaionArray == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList):malloc \"newStaionArray\" error.\n ");
        #endif
        return -1;
    }

    memset(newStaionArray, 0, totalStations*5);
    totalStations = 0;
    for(i=0;i<arrayLines;i++)
    {
        p = sl->next;
        while( p != NULL)
        {
            if(!strcmp(p->name, stationFileNames[i]))
            {
                int j;
                for(j=0; j< p->stationNum; j++)
                {
                    if( (p->station)[j] == NULL )
                    {
                        #ifdef DEBUG
                        printf("Fun(mergeStationList): (p->station)[j] is NULL.\n ");
                        #endif
                        return -1;
                    }
                    strcpy(newStaionArray[totalStations+j],(p->station)[j]);
                }
                totalStations += p->stationNum;
                break;
            }

            p = p->next;
        }
    }

    int deleteNum = 0;
    for(i=0;i<totalStations-1;i++)
    {
        for(j=i+1;j<totalStations;j++)
        {
            if(!strcmp(newStaionArray[i], newStaionArray[j]) )
            {
                memset(newStaionArray[i], 0, 5);
                deleteNum++;
            }
        }
    }

    StationNode * newStationNode = (StationNode *)malloc(sizeof(StationNode));
    if( newStationNode == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"newStationNode\" error.\n ");
        #endif
        return -1;
    }
    memset(newStationNode, 0, sizeof(StationNode));

    char (*mergeStationArray)[5] = (char **)malloc((totalStations - deleteNum)*5);
    if( mergeStationArray == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"mergeStationArray\" error.\n ");
        #endif
        return -1;
    }
    memset(mergeStationArray, 0, (totalStations - deleteNum)*5);

    j=0;
    for(i=0;i<totalStations;i++)
    {
        if(strlen(newStaionArray[i]))
        {
            strcpy(mergeStationArray[j++], newStaionArray[i]);
        }

    }

    newStationNode->name = (char *)malloc(strlen(name)+1);
    if( newStationNode->name == NULL )
    {
        #ifdef DEBUG
        printf("Fun(mergeStationList): malloc \"newStationNode->name\" error.\n ");
        #endif
        return -1;
    }
    memset(newStationNode->name, 0 ,strlen(name)+1);
    strcpy(newStationNode->name,name);

    newStationNode->station = mergeStationArray;
    newStationNode ->stationNum = j;
    p = sl->next;
    sl->next = newStationNode;
    newStationNode->next = p;

}

/**************************************************************************************************
*    function   :   reload station list after merging stations.
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int stationListReload()
{
    DownInfo *p = downInfoList->next;
    StationNode *psl;
    int i=0,j=0;


    while( p != NULL )
    {

        i=0; j=0;
        psl = sl->next;

        while(psl != NULL)
        {
            if( ( psl->name == NULL ) || (p->stationList == NULL) )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): psl->name or p->stationList is NULL.\n");
                #endif
                return -1;
            }
            if(!strcmp(p->stationList, psl->name))// not equal to
            {
                break;
            }

            psl  = psl->next;
        }


        if( psl == NULL)
        {
            //get file number according to the character ','
            while( (p->stationList)[i] )
            {
                if((p->stationList)[i] == ',')
                {
                    j++;
                }
                i++;
            }

            //allocate memory
            char (*stationFileName)[MAX_STATION_FILE_NAME_SIZE] = (char **)malloc((j+1)*MAX_STATION_FILE_NAME_SIZE);
            if( stationFileName == NULL )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): malloc \"stationFileName\" error..\n");
                #endif
                return -1;
            }
            memset(stationFileName, 0, (j+1)*MAX_STATION_FILE_NAME_SIZE);

            char *name = malloc(strlen(p->stationList)+1);
            if( stationFileName == NULL )
            {
                #ifdef DEBUG
                printf("Fun(stationListReload): malloc \"name\" error..\n");
                #endif
                return -1;
            }
            memset(name, 0, strlen(p->stationList)+1);
            strcpy(name, p->stationList);

            strcpy(stationFileName[0], strtok(p->stationList,","));

            for(i = 1; i <= j; i++)
            {
                strcpy(stationFileName[i], strtok(NULL,","));
            }

            mergeStationList(name, stationFileName, j+1);
            free(stationFileName);
            free(name);

        }

        p = p->next;
    }
}

/**************************************************************************************************
*    function   :   reload station list after merging stations.
*    para       :   {char *conf} system.ini path
*
*    return     :   {int} -1:error;0:ok.
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
int initList()
{
    downInfoList = (DownInfo*) malloc(sizeof(DownInfo));
    if( downInfoList == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"downInfoList\" error..\n");
        #endif
        return -1;
    }
    memset(downInfoList, 0, sizeof(DownInfo));
    downInfoList ->next = NULL;

    fs=(FtpServer*)malloc(sizeof(FtpServer));
    if( fs == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"fs\" error..\n");
        #endif
        return -1;
    }
    memset(fs, 0, sizeof(FtpServer));
    fs->next = NULL;

    sl = (StationNode *)malloc(sizeof(StationNode));
    if( sl == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"sl\" error..\n");
        #endif
        return -1;
    }
    memset(sl,0,sizeof(StationNode));
    sl->next = NULL;

    downloadList=(DownloadNode*)malloc(sizeof(DownloadNode));
    if( downloadList == NULL )
    {
        #ifdef DEBUG
        printf("Fun(stationListReload): malloc \"downloadList\" error..\n");
        #endif
        return -1;
    }
    memset(downloadList, 0, sizeof(DownloadNode));
    downloadList->next = NULL;

}

/**************************************************************************************************
*    function   :   tranverse Station List
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
void tranverseStationList()
{
    StationNode *p = sl->next;

    while(p != NULL)
    {
        int i = p->stationNum;

        printf("i : %d,\tname : %s\n",i,p->name);
        while(i--)
        {
            //printf("%d\t%s\n",i,(p->station)[i]);
        }

        p = p->next;
    }
}

/**************************************************************************************************
*    function   :   tranverse ftp server list
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.8.9 wujun} frist create;
**************************************************************************************************/
void tranverseFtpServerList()
{
    FtpServer *p = fs->next;

    while(p != NULL)
    {
        //printf("%s\t%d\t%s\t%s\t\n",p->ip,p->port,p->username,p->passwd);
        p = p->next;
    }
}


