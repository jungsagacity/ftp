#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "global.h"

#include "download.h"

char    hourly_type[13]= {'D','N','G','L','R','M','T','J','A','K','I','E'}; //Type of  hourly file
char    highrate_type[7]=  {'D','N','G','L','R','M'}; //Type of highrate file
int     tmp=-1;


DownloadList downloadList[MAX_DOWNLOAD_TASK_NUM]; //global variable£¬record each data's  DC_path AC_path and state

int     downloadList_numb; //record the numb of the data from 00:00 to current


/******************************************************************************************
*    function   :   the module control function ,make sure all the function good running
*    para       :   {NULL}
*
*    return     :   {void}
*    history    :   {2013.7.27 yangyang} frist create;
******************************************************************************************/

void module_control()
{

        MYtime mt;
        MYtime *pmt = &mt;
        get_currentime(pmt);    //get current time
        int test=-1;
        if(mt.minute==10||mt.minute==25||mt.minute==40||mt.minute==55)//the condition to check the file whether exist
        {
            if(test!=mt.minute)
            {
			    test=mt.minute;
                if(mt.minute==25&&mt.hour==0)//at the begin of a day ,init the global array
                {
                    downloadList_numb=0;
					int i;
					for(i=0;i<MAX_DOWNLOAD_TASK_NUM;i++)
					{
					    strcpy(downloadList[i].remote_filename,"\0");
						strcpy(downloadList[i].local_filename,"\0");
						downloadList[i].state=DOWNLOAD_FILE_DEFAULT;
					}
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
                if(minute%15==0)
                {
                    char download_list[20][MAX]= {'\0'};
                    int  undownload_numb=0;
                    int  tmp=creat_current_direct(year,day,hour,minute,download_list);//creat download_list
                    undownload_numb=creat_logfile(year,day,hour,minute,download_list,tmp);//updata downloadList_numb
                    #ifdef DEBUG
					printf("new data numbs£º%d  lost_data numbs£º %d  \n\n",tmp,undownload_numb);
				    #endif
                }
            }


    }
}


/******************************************************************************************
*    function   :   according the download list to check all the records exist or not
                       && write the info and state to the global var
*    para       :   {int year,int day,int hour,int minute,char *download_list[]}
*
*    return     :   {int} return the file nubms that not exist in currentime
*    history    :   {2013.7.26 yangyang} frist create;
******************************************************************************************/

int creat_logfile(int year,int day,int hour,int minute,char download_list[][MAX],int tmp)
{
    int i,k=0;//k,return the numb of the  files that not exist
    int flag=0;
	#ifdef DEBUG
	    FILE *fp=fopen("/home/jung/tmp/FromDC/logs/212.txt","a+");
		fprintf(fp,"the transfer time of current files :%d %d %d:%d\n",year,day,hour%24,minute);
    #endif


    for(i=0; i<tmp; i++)
    {
        flag=Search_undownload_file(download_list[i]);//check the file exist or not
        strcpy(downloadList[downloadList_numb+i].local_filename,download_list[i]);//set the AC_path
        str_copy(downloadList[downloadList_numb+i].remote_filename,
                 downloadList[downloadList_numb+i].local_filename,strlen(DW_ANALYSIS_CENTER_PATH_PREFIX));//set the DC_path
        downloadList[downloadList_numb+i].state=flag;
        #ifdef DEBUG
        if(downloadList[i+downloadList_numb].state== DOWNLOAD_FILE_NONEXIST )
        {
            k++;
        }
		fprintf(fp,"%s %d\n",download_list[i],flag); //write the logs
        #endif
    }
    downloadList_numb+=tmp;
	#ifdef DEBUG
	    fclose(fp);
    #endif

    return k;
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

	#ifdef DEBUG
    if(tmp!=mt->minute)
    {
        printf ( "Current date/time: %s", asctime (currentime));
        //the condition to write log file
        if(mt->minute==10||mt->minute==25||mt->minute==40||mt->minute==55)
        {
            FILE *fp=fopen("/home/jung/tmp/FromDC/logs/212.txt","a+");
            fprintf(fp,"The current data/time if:%s",asctime(currentime));
            fclose(fp);
        }
        tmp=mt->minute;
    }
     #endif
}

/******************************************************************************************
*    function   :   creat the current downlist
*    para       :   {int year ,int day ,int hour, int minute ,char *download_list[]}
*                          *download[]store the current filename that should exist in local
*    return     :   int ,    return the numb of the current downlist
*    history    :   {2013.7.24 yangyang} frist create;
******************************************************************************************/

int creat_current_direct(int year,int day,int hour,int minute,char download_list[][MAX])
{
    int k=0;//return the numb of the file creat in current time
    //fint the day root direct according to the time
    char  current_root_direct[MAX];
    strcpy(current_root_direct,DW_ANALYSIS_CENTER_PATH_PREFIX);
    char year_direct[5]= {'\0'};
    itoa(year,year_direct,10);
    strcat(current_root_direct,year_direct);
    strcat(current_root_direct,"/");
    char day_direct[5]= {'\0'};
    three_day(day,day_direct);
    strcat(current_root_direct,day_direct);

    //creat hourly_file
    if(minute==0)
    {
        hour=(hour+24-1)%24;
        char current_hourly_direct[MAX]= {'\0'};
        strcpy(current_hourly_direct,current_root_direct);
        strcat(current_hourly_direct,Hourly);
        int current_type_hourly;
        for(current_type_hourly=0; current_type_hourly<strlen(hourly_type); current_type_hourly++)
        {
		    //add the type direct
            char current_type_direct1[MAX]= {'\0'};
            char current_type_tmp1[2]= {'\0'};
            current_type_tmp1[0]=hourly_type[current_type_hourly];
            strcpy(current_type_direct1,current_hourly_direct);
            strcat(current_type_direct1,"/");
            strcat(current_type_direct1,current_type_tmp1);

            //creat  the current list
			//add the hour direct
            char current_type_hour_direct1[MAX]= {'\0'};
            char current_type_hour_tmp1[3]= {'\0'};
            two_hour(hour,current_type_hour_tmp1);
            strcpy(current_type_hour_direct1,current_type_direct1);
            strcat(current_type_hour_direct1,"/");
            strcat(current_type_hour_direct1,current_type_hour_tmp1);

            //creat the file name
            char current_file_hourname[20]= {'\0'};
            creat_hourly_filename(year,day,hour,current_type_hourly,current_file_hourname);//creaet hourly filename
            char current_file_hourdirect[MAX]= {'\0'};
            strcpy(current_file_hourdirect,current_type_hour_direct1);
            strcat(current_file_hourdirect,"/");
            strcat(current_file_hourdirect,current_file_hourname);
            strcpy(download_list[k++],current_file_hourdirect);

        }
    }
    //creat highrate_file
    minute=(minute+60-15)%60;
    char current_highrate_direct[MAX]= {'\0'};
    strcpy(current_highrate_direct,current_root_direct);
    strcat(current_highrate_direct,Highrate);
    int current_type_highrate;
    for(current_type_highrate=0; current_type_highrate<strlen(highrate_type); current_type_highrate++)
    {
	    //add the type direct
        char current_type_direct2[MAX]= {'\0'};
        char current_type_tmp2[2]= {'\0'};
        current_type_tmp2[0]=highrate_type[current_type_highrate];
        strcpy(current_type_direct2,current_highrate_direct);
        strcat(current_type_direct2,"/");
        strcat(current_type_direct2,current_type_tmp2);

        //creat the current list
        //add the hour direct
        char current_type_hour_direct2[MAX]= {'\0'};
        char current_type_hour_tmp2[3]= {'\0'};
        two_hour(hour,current_type_hour_tmp2);
        strcpy(current_type_hour_direct2,current_type_direct2);
        strcat(current_type_hour_direct2,"/");
        strcat(current_type_hour_direct2,current_type_hour_tmp2);

        //add the minute direct
        char current_type_hour_highrate_direct[MAX]= {'\0'};
        char current_type_hour_tmp3[3]= {'\0'};
        two_minute(minute,current_type_hour_tmp3);
        strcpy(current_type_hour_highrate_direct,current_type_hour_direct2);
        strcat(current_type_hour_highrate_direct,"/");
        strcat(current_type_hour_highrate_direct,current_type_hour_tmp3);

        //creat the file name
        char current_file_highratename[20]= {'\0'};
        creat_highrate_filename(year,day,hour,minute,current_type_highrate,current_file_highratename);//creat highrate filename
        char current_file_highratedirect[MAX]= {'\0'};
        strcpy(current_file_highratedirect,current_type_hour_highrate_direct);
        strcat(current_file_highratedirect,"/");
        strcat(current_file_highratedirect, current_file_highratename);
        strcpy(download_list[k++],current_file_highratedirect);
    }
	#ifdef DEBUG
		printf("\n(minite=0£¬15m->transfer£¬25m->check£¬minute=45£¬0m->transger(hourly and highrate files)\n");
        printf("\n the time remark the filename£º%d %d %d:%d\n",year,day,hour,minute);
    #endif

    return k;
}



/******************************************************************************************
*    function   :   creat the highrate file name
*    para       :   {int year,int day,int hour,int minute,int type,char *highrate_filename}
*                      *highrate_filename  store the filename,type is the type of the file
*
*    return     :   {void}
*    history    :   {2013.7.23 yangyang} frist create;
******************************************************************************************/


void creat_highrate_filename(int year,int day,int hour,int minute,int type,char highrate_filename[])
{
    strcpy(highrate_filename,DATA_SATION);//add the DC_station
    char tmp1[5]= {'\0'};
    three_day(day,tmp1);
    strcat(highrate_filename,tmp1); //add the day in the year
    highrate_filename[7]=hour+97;   //change the hour too one char range from  a to x
    char tmp2[5]= {'\0'};
    two_minute(minute,tmp2);
    highrate_filename[8]=tmp2[0];
    highrate_filename[9]=tmp2[1];   //add the minute
    highrate_filename[10]='.';
    char tmp3[5]= {'\0'};
    two_year(year,tmp3);
    highrate_filename[11]=tmp3[0];
    highrate_filename[12]=tmp3[1];   //add the last two char of the year
    highrate_filename[13]=highrate_type[type];//add the type of file
    highrate_filename[14]='.';
    highrate_filename[15]='Z';//filename extension
    highrate_filename[16]='\0';
	//printf("%s\n",highrate_filename);
}

/******************************************************************************************
*    function   :   creat the hourly file name
*    para       :   {int year,int day,int hour,int type,char *hour_filename}
*
*    return     :   {void}
*    history    :   {2013.7.23 yangyang} frist create;
******************************************************************************************/

void creat_hourly_filename(int year,int day,int hour,int type,char hour_filename[])
{
    strcpy(hour_filename,DATA_SATION);
    char tmp1[5]= {'\0'};
    three_day(day,tmp1);
    strcat(hour_filename,tmp1);
    hour_filename[7]=hour+97;
    hour_filename[8]='.';
    char tmp2[5]= {'\0'};
    two_year(year,tmp2);
    hour_filename[9]=tmp2[0];
    hour_filename[10]=tmp2[1];
    hour_filename[11]=hourly_type[type];
    hour_filename[12]='.';
    hour_filename[13]='Z';
    hour_filename[14]='\0';
    //printf("%s\n",hour_filename);
}

/*******************************************************************************
*    function   :   get the last two char of the year
*    para       :   {int year,char *y};(like year=2013,y="13")
*
*    return     :   {void}
*    history    :   {2013.7.20 yangyang} frist create;
*******************************************************************************/

void two_year(int year,char y[])
{
    int mode_year;
    mode_year=year%100;
    sprintf(y,"%02d",mode_year);
}


/******************************************************************************
*    function   :   transfer the (int) day to char *
*    para       :   {int day,char *y};(int)day change to three char store in y
                                      (like day=123,y="113" or day=9,y="009")
*
*    return     :   {void}
*    history    :   {2013.7.20 yangyang} frist create;
******************************************************************************/

void three_day(int day,char d[])
{
    sprintf(d,"%03d",day);
    /*
    char tmp[5]= {'\0'};
    itoa(day,tmp,10);
    if(day<10)
    {
        strcpy(d,"00");
        strcat(d,tmp);
    }
    else if(day<100)
    {
        strcpy(d,"0");
        strcat(d,tmp);
    }
    else
    {
        strcpy(d,tmp);
    }*/
}


/*****************************************************************************
*    function   :   transfer the (int) hour to char *
*    para       :   {int day,char *y};(int)hour change to two char store in h
                                      (like hour=12,y="12" or hour=0,y="00")
*
*    return     :   {void}
*    history    :   {2013.7.20 yangyang} frist create;
******************************************************************************/

void  two_hour(int hour, char h[])
{
    sprintf(h,"%02d",hour);
    /*
    char tmp[5]= {'\0'};
    itoa(hour,tmp,10);
    if(hour<10)
    {
        strcpy(h,"0");
        strcat(h,tmp);
    }
    else
        strcpy(h,tmp);
    */
}

/**********************************************************************************
*    function   :   transfer the (int) minute to char *
*    para       :   {int minute,char *y}(like,minute=0,15,30,25,m="00""15""30""45")
*
*    return     :   {void}
*    history    :   {2013.7.20 yangyang} frist create;
***********************************************************************************/

void two_minute(int minute,char m[])
{
    if(minute==0)
        strcpy(m,"00");
    else
        itoa(minute,m,10);
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

int Search_undownload_file(char filename[])
{
    int k=0;
    k=access(filename,0);
    if( k == 0)
        return DOWNLOAD_FILE_EXIST	;   //exist
    else
        return DOWNLOAD_FILE_NONEXIST;  //non_exist
}

/*****************************************************************************************
*    function   :   transfer   int to char *
*    para       :   {int tmpInt ,char *str,int dec}(like ,dec =10,mean transfer as decimal
*
*    return     :   int
*    history    :   {2013.7.22 yangyang} frist create;
*******************************************************************************************/

int itoa(int temp, char * str, int dec) //int to char*
{
    sprintf(str,"%d",temp);
}

/*****************************************************************************************
*    function   :   the user_defined copy of char*
*    para       :   {char *A, char *B,int begin}
                          copy the char in B from the begin and add it to the end of A
*    return     :   void
*    history    :   {2013.7.22 yangyang} frist create;
*******************************************************************************************/

void str_copy(char A[],char B[],int i)
{
    int j=0;
    char tmp[MAX]= {'\0'};
    while(B[i]!='\0')
    {
        tmp[j++]=B[i++];
    }
    strcpy(A,DATA_CENTER_PATH_PREFIX);//DATA_CENTER_PATH_PREFIX is the root direct of DC
    strcat(A,tmp);
}
