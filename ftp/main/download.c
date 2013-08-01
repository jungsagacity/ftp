#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "global.h"

#include "download.h"

//#include <pthread.h>
//#include "ftp.h"
//extern int socket_control;
//                             
//#define   LOCAL_DATA_SRC  "/home/jung/tmp/FromDC/"          //The local root path  
//#define   LOGDIRECT       "/home/jung/tmp/FromDC/logs/"    //Direct of the logs
//#define   IGMAS           "download/iGMAS/"                //The ftp(Data_Centre) root path 
//#define   ssss            "ssss"               //The code name of  theDC_station                  


 
char   hourly_type[13]= {'D','N','G','L','R','M','T','J','A','K','I','E'}; //Type of  hourly file
char   highrate_type[7]=  {'D','N','G','L','R','M'}; //Type of highrate file
int tmp=-1;


DownloadList downloadList[MAX_DOWNLOAD_TASK_NUM]; //ȫ�ֱ���������ÿ�����ݵ�����״̬

int     downloadList_numb; //��¼�����¼������





int itoa(int tempInt, char * str, int dec) //int to char*
{
    sprintf(str,"%d",tempInt);
}

void str_copy(char A[],char B[],int i)//��B�ӵ�i���ַ��ӵ���igmas�����Ƹ�A
{
    int j=0;
    char tmp[MAX]= {'\0'};
    while(B[i]!='\0')
    {
        tmp[j++]=B[i++];
    }
    strcpy(A,DATA_CENTER_PATH_PREFIX);
    strcat(A,tmp);
}

void module_control()
{
        
        MYtime mt;
        MYtime *pmt = &mt;
        get_currentime(pmt);    //get current time
        int test=-1;
        if(mt.minute==10||mt.minute==25||mt.minute==40||mt.minute==55)//��ʱ��������ʱ������
        {
            if(test!=mt.minute)
            {
			    test=mt.minute;
                if(mt.minute==25&&mt.hour==0)//��鵱���һ���ļ� ���쿪ʼ����¼���;
                {
                    downloadList_numb=0;
					int i;
					for(i=0;i<MAX_DOWNLOAD_TASK_NUM;i++)
					{
					    strcpy(downloadList[i].remote_filename,"\0");
						strcpy(downloadList[i].local_filename,"\0");
						downloadList[i].state=0;
					}
                }
                //mt�б������ʱ�䣬����ת��Ϊ�ļ���ʼ����ʱ�䣬
                int year,day,hour,minute;
                minute=mt.minute-10;
                if(minute==0)
                {
                    if(mt.hour==0&&mt.day==1)
                    {
                        year=mt.year-1;
                        hour=24;                   //�ڴ���00:00ʱ��ת����24:00
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
                    int  tmp=creat_current_direct(year,day,hour,minute,download_list);//����ȫ���б�        
                    undownload_numb=creat_logfile(year,day,hour,minute,download_list,tmp); //����log_data   
                    #ifdef DEBUG
					printf("��ǰ��������������%d  ȱʧ���������� %d  \n\n",tmp,undownload_numb);
				    #endif
                }
            }

        
    }
}

int creat_current_direct(int year,int day,int hour,int minute,char download_list[][MAX])//������ǰʱ�������б�(current)
{
    int k=0;//���ص�ǰ�ļ�����
    //����ʱ���ҵ���ѯ�ĸ�·��
    char  current_root_direct[MAX];
    strcpy(current_root_direct,DW_ANALYSIS_CENTER_PATH_PREFIX);
    char year_direct[5]= {'\0'};
    itoa(year,year_direct,10);
    strcat(current_root_direct,year_direct);
    strcat(current_root_direct,"/");
    char day_direct[5]= {'\0'};
    three_day(day,day_direct);
    strcat(current_root_direct,day_direct);
    //�ҵ���ÿ�����ʼ·����Ȼ���Сʱ�ļ���ʮ������ļ�����
    //hourly_file
    if(minute==0)
    {
        hour=(hour+24-1)%24;
        char current_hourly_direct[MAX]= {'\0'};
        strcpy(current_hourly_direct,current_root_direct);
        strcat(current_hourly_direct,Hourly);
        int current_type_hourly;
        for(current_type_hourly=0; current_type_hourly<strlen(hourly_type); current_type_hourly++)
        {
		    //СʱĿ¼������Ŀ¼·��
            char current_type_direct1[MAX]= {'\0'};            
            char current_type_tmp1[2]= {'\0'};
            current_type_tmp1[0]=hourly_type[current_type_hourly];
			
            strcpy(current_type_direct1,current_hourly_direct);
            strcat(current_type_direct1,"/");
            strcat(current_type_direct1,current_type_tmp1);
			
            //��鵱ǰʱ�̴���������Ƿ�ɹ�(hour_file) 
			//СʱĿ¼������Ŀ¼��Сʱ·��
            char current_type_hour_direct1[MAX]= {'\0'};    
            char current_type_hour_tmp1[3]= {'\0'};
            two_hour(hour,current_type_hour_tmp1);
            strcpy(current_type_hour_direct1,current_type_direct1);
            strcat(current_type_hour_direct1,"/");
            strcat(current_type_hour_direct1,current_type_hour_tmp1);
			
            //·��������ϣ����������ļ���
            char current_file_hourname[20]= {'\0'};
            creat_hourly_filename(year,day,hour,current_type_hourly,current_file_hourname);//creaet hourly filename
            char current_file_hourdirect[MAX]= {'\0'};
            strcpy(current_file_hourdirect,current_type_hour_direct1);
            strcat(current_file_hourdirect,"/");
            strcat(current_file_hourdirect,current_file_hourname);
            strcpy(download_list[k++],current_file_hourdirect);

        }
    }
    //highrate_file
    minute=(minute+60-15)%60;
    char current_highrate_direct[MAX]= {'\0'};
    strcpy(current_highrate_direct,current_root_direct);
    strcat(current_highrate_direct,Highrate);
    int current_type_highrate;
    for(current_type_highrate=0; current_type_highrate<strlen(highrate_type); current_type_highrate++)
    {
	    //ʮ�����Ŀ¼������Ŀ¼·��
        char current_type_direct2[MAX]= {'\0'};            
        char current_type_tmp2[2]= {'\0'};
        current_type_tmp2[0]=highrate_type[current_type_highrate];
        strcpy(current_type_direct2,current_highrate_direct);
        strcat(current_type_direct2,"/");
        strcat(current_type_direct2,current_type_tmp2);
		
        //��鵱ǰʱ�̴���������Ƿ�ɹ�(highrate_file)
        //����Сʱ·��
        char current_type_hour_direct2[MAX]= {'\0'};
        char current_type_hour_tmp2[3]= {'\0'};
        two_hour(hour,current_type_hour_tmp2);
        strcpy(current_type_hour_direct2,current_type_direct2);
        strcat(current_type_hour_direct2,"/");
        strcat(current_type_hour_direct2,current_type_hour_tmp2);
		
        //�������·��
        char current_type_hour_highrate_direct[MAX]= {'\0'};
        char current_type_hour_tmp3[3]= {'\0'};
        two_minute(minute,current_type_hour_tmp3);
        strcpy(current_type_hour_highrate_direct,current_type_hour_direct2);
        strcat(current_type_hour_highrate_direct,"/");
        strcat(current_type_hour_highrate_direct,current_type_hour_tmp3);
		
        //·��������ϣ����������ļ���
        char current_file_highratename[20]= {'\0'};
        creat_highrate_filename(year,day,hour,minute,current_type_highrate,current_file_highratename);//creat highrate filename
        char current_file_highratedirect[MAX]= {'\0'};
        strcpy(current_file_highratedirect,current_type_hour_highrate_direct);
        strcat(current_file_highratedirect,"/");
        strcat(current_file_highratedirect, current_file_highratename);
        strcpy(download_list[k++],current_file_highratedirect);
    }
	#ifdef DEBUG
		printf("\n(minite=0��15m->transger��25m->check��minute=45��0m->transger(hourly and highrate files)\n");
        printf("\n the time remark the filename��%d %d %d:%d\n",year,day,hour,minute);		
    #endif

    return k;
}


int creat_logfile(int year,int day,int hour,int minute,char download_list[][MAX],int tmp)//��־��¼���޸���־��¼���飬��д��txt�ļ�����
{
    int i,k=0;//k������Ҫ���ص��ļ�����
    int flag=0;
    /*
    //����������־·������
    char log_name[MAX]= {'\0'};
    char file_year[10]= {'\0'};
    char file_day[5]= {'\0'};
    three_day(day ,file_day);
    itoa(year,file_year,10);
    strcpy(log_name,LOGDIRECT);
    strcat(log_name,file_year);
    strcat(log_name,file_day);
    strcat(log_name,".txt");
    //printf("logfilename��%s\n",log_name);
    //printf("downloadList_numb=%d\n",downloadList_numb);
    */
	#ifdef DEBUG
	    FILE *fp=fopen("/home/jung/tmp/FromDC/logs/212.txt","a+");
		fprintf(fp,"the transfer time of current files��%d %d %d:%d\n",year,day,hour%24,minute);		
    #endif
    
    
    for(i=0; i<tmp; i++)
    {
        flag=Search_undownload_file(download_list[i]);//�ж��Ƿ����
        strcpy(downloadList[downloadList_numb+i].local_filename,download_list[i]);
        str_copy(downloadList[i].remote_filename,downloadList[i].local_filename,strlen(DW_ANALYSIS_CENTER_PATH_PREFIX));
        downloadList[downloadList_numb+i].state=flag;

        if(downloadList[i+downloadList_numb].state==-1)
        {
            k++;
        }
		#ifdef DEBUG
            fprintf(fp,"%s %d\n",downloadList[i].local_filename,downloadList[i].state);  //д����־
        #endif
    }
	#ifdef DEBUG	
	    fclose(fp);
    #endif
    downloadList_numb+=tmp;
    return k;
}

void get_currentime(MYtime *mt)//��ȡ��ǰʱ�䣬��ת��������Ҫ����ʽ
{
    struct tm *currentime;
    time_t t;
    time(&t);
    currentime=localtime(&t);
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

void creat_hourly_filename(int year,int day,int hour,int type,char hour_filename[])//����Сʱ�ļ���(������·��)
{
    strcpy(hour_filename,DATA_SATION);//strcat the DC_station
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

void creat_highrate_filename(int year,int day,int hour,int minute,int type,char highrate_filename[])//creat highrate_filename
{
    strcpy(highrate_filename,DATA_SATION);//strcat the DC_station
    char tmp1[5]= {'\0'};
    three_day(day,tmp1);
    strcat(highrate_filename,tmp1); //strcat the day in the year
    highrate_filename[7]=hour+97;
    char tmp2[5]= {'\0'};
    two_minute(minute,tmp2);
    highrate_filename[8]=tmp2[0];
    highrate_filename[9]=tmp2[1];
    highrate_filename[10]='.';
    char tmp3[5]= {'\0'};
    two_year(year,tmp3);
    highrate_filename[11]=tmp3[0];
    highrate_filename[12]=tmp3[1];
    highrate_filename[13]=highrate_type[type];
    highrate_filename[14]='.';
    highrate_filename[15]='Z';
    highrate_filename[16]='\0';
	//printf("%s\n",highrate_filename);
}

/**
*    function   :   log all information including error;
*    para       :   {char * msg} actural message;
*
*    return     :   {void}
*
*    history    :   {2013.7.18 wujun} frist create;
**/

/**
 *      function    :   fill host ip and port
 *      para        :   {char *host_ip_addr}   like "127.0.0.1"
                        {struct sockaddr_in *host} ftp server add info to being filled that used to connect to ftp server
                        {int port} ftp server port,default 21;
        return      :   {int} error code;
        history     :   {2013.7.18 wujun} fristly be created
**/

//int fill_host_addr(char *host_ip_addr,struct sockaddr_in *host,int port)
/*************************************************************************************
*    function   :   get the last two char of the year
*    para       :   {int year,char *y}; the two char store in y(like year=2013,y="13")
*
*    return     :   {void}
*
*    history    :   {2013.7.20 yangyang} frist create;
**************************************************************************************/

void two_year(int year,char y[])  //get the last two char of the year
{
    int mode_year;
    mode_year=year%100;
    sprintf(y,"%02d",mode_year);
}

void three_day(int day,char d[])//change the day to char*
{
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
    }
}

void  two_hour(int hour, char h[])//change the hour to char *
{
    char tmp[5]= {'\0'};
    itoa(hour,tmp,10);
    if(hour<10)
    {
        strcpy(h,"0");
        strcat(h,tmp);
    }
    else
        strcpy(h,tmp);
}

void two_minute(int minute,char m[])//change the minute to char *
{
    if(minute==0)
        strcpy(m,"00");
    else
        itoa(minute,m,10);
}

int transfer(int year,int month,int day)    //change the data to the day in the year
{
    int a[12]= {31,28,31,30,31,30,31,31,30,31,30,31};  //Days of every month
    int i,sum=0;                                       //sum is the 
    for(i=0; i<month-1; i++)
    {
        sum+=a[i];
    }
    sum+=day;
    if(year%4==0&&year%100!=0&&month>2) sum++;
    return sum;
}



int Search_undownload_file(char filename[])         //check the file exist or  not
{
    int k=0;
    k=access(filename,0);
    if( k == 0)
        return DOWNLOAD_FILE_EXIST	;
    else
        return DOWNLOAD_FILE_NONEXIST;
}






/*
void download()
{
    int i =0;
    int ftperror = 0;
	char con[MAX]={'\0'};
    while(1)
    {
        sleep(1);
        for(; i < MAX; i++)
        {	
			//sprintf(con,"L:%s\tR:%s\t%d",log_data[i].local_filename, log_data[i].remote_filename, log_data[i].state);
			//plog(con);
			//memset(con,'\0',MAX);
            if(log_data[i].state == -1)
            {
                if ( connectFtpServer("192.168.0.222", 21, "public", "123456") > 0 )
                {
                    if( (ftperror = ftp_get(log_data[i].remote_filename, log_data[i].local_filename, socket_control) ) == 0)
                    {
                        printf("\n%s----download success.\n",log_data[i].remote_filename);
                        log_data[i].state = 2;
                    }
                    else
                    {
                        printf("\n%s----download fail.\n",log_data[i].remote_filename);
                        log_data[i].state = 3;
                    }

                    close(socket_control);
                }
                else
                {
                    printf("connecting error...");
                    log_data[i].state = 3;
                }
            }
        }
        i = 0;

    }


}
int  main()
{
    pthread_t downloadTread;
    pthread_t controlTread;
    pthread_create(&downloadTread,NULL,download,(void *)NULL);
    pthread_create(&controlTread,NULL,module_control,(void *)NULL);
    while(1);
    return 0;
}
*/