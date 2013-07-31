#include<unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "download.h"


int itoa(int tempInt, char * str, int dec)
{
    sprintf(str,"%d",tempInt);
}


void str_copy(char A[],char B[],int i)//把B从第i个字符加到”igmas“后复制给A
{
    int j=0;
    char tmp[1000]= {'\0'};
    while(B[i]!='\0')
    {
        tmp[j++]=B[i++];
    }
    strcpy(A,IGMAS);
    strcat(A,tmp);
}


void module_control()//时钟控制
{
    int test=-1;
    while(1)
    {
        MYtime mt;
        get_currentime(mt);    //获取当前时间
        if(mt.minute==10||mt.minute==25||mt.minute==40||mt.minute==55)//定时检查成立的时间条件
        {

            if(test!=mt.minute)
            {
                test=mt.minute;
                int year,day,hour,minute;
                minute=mt.minute-10;
                if(minute==0)
                {
                    if(mt.hour==0&&mt.day==1)
                    {
                        year=mt.year-1;
                        hour=24;      //在处理00:00时，转化成24:00
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
                    char download_list[1000][1000]= {'\0'};
                    char undownload_list[1000][1000]= {'\0'};
                    int undownload_numb=0;
                    log_data_numb=0;
                    log_data_numb=creat_current_direct(year,day,hour,minute,download_list);//生成全部列表
                    undownload_numb=creat_logfile(year,day,hour,minute,download_list,undownload_list);//生成下载列表
                    int i;
                    for(i=0; i<undownload_numb; i++)
                    {
                        //printf("%s\n",undownload_list[i]);
                    }

                }
            }

        }
    }
}

int creat_current_direct(int year,int day,int hour,int minute,char download_list[][1000])//创建当前时刻下载列表(00:00~~current)
{
    int k=0;//返回文件个数
    //根据时间找到查询的根路径
    char  current_root_direct[1000];
    strcpy(current_root_direct,ROOTDIRECT);
    char year_direct[5]= {'\0'};
    itoa(year,year_direct,10);
    strcat(current_root_direct,year_direct);
    strcat(current_root_direct,"/");
    char day_direct[5]= {'\0'};
    three_day(day,day_direct);
    strcat(current_root_direct,day_direct);
    //找到了每天的起始路径，然后分小时文件和十五分钟文件查找
    //hourly_file
    char current_hourly_direct[1000]= {'\0'};
    strcpy(current_hourly_direct,current_root_direct);
    strcat(current_hourly_direct,Hourly);
    int current_type_hourly;
    for(current_type_hourly=0; current_type_hourly<numb1; current_type_hourly++)
    {
        char current_type_direct1[1000]= {'\0'};            //小时目录下类型目录路径
        char current_type_tmp1[2]= {'\0'};
        current_type_tmp1[0]=t1[current_type_hourly];
        strcpy(current_type_direct1,current_hourly_direct);
        strcat(current_type_direct1,"/");
        strcat(current_type_direct1,current_type_tmp1);
        //版本二，检查当前时刻之前的当天内传输的数据是否成功
        int hour1;
        for(hour1=0; hour1<hour; hour1++)
        {
            char current_type_hour_direct1[1000]= {'\0'};     //小时目录下类型目录下小时路径
            char current_type_hour_tmp1[3]= {'\0'};
            two_hour(hour1,current_type_hour_tmp1);
            strcpy(current_type_hour_direct1,current_type_direct1);
            strcat(current_type_hour_direct1,"/");
            strcat(current_type_hour_direct1,current_type_hour_tmp1);
            //路径生成完毕，下面生成文件名
            char current_file_hourname[20]= {'\0'};
            creat_hourly_filename(year,day,hour1,current_type_hourly,current_file_hourname);//生成filename
            char current_file_hourdirect[1000]= {'\0'};
            strcpy(current_file_hourdirect,current_type_hour_direct1);
            strcat(current_file_hourdirect,"/");
            strcat(current_file_hourdirect,current_file_hourname);
            strcpy(download_list[k++],current_file_hourdirect);
        }
    }
    //highrate_file
    char current_highrate_direct[1000]= {'\0'};
    strcpy(current_highrate_direct,current_root_direct);
    strcat(current_highrate_direct,Highrate);
    int current_type_highrate;
    for(current_type_highrate=0; current_type_highrate<numb2; current_type_highrate++)
    {
        char current_type_direct2[1000]= {'\0'};            //十五分钟目录下类型目录路径
        char current_type_tmp2[2]= {'\0'};
        current_type_tmp2[0]=t2[current_type_highrate];
        strcpy(current_type_direct2,current_highrate_direct);
        strcat(current_type_direct2,"/");
        strcat(current_type_direct2,current_type_tmp2);
        //版本二，检查当前时刻之前的当天内传输的数据是否成功
        int hour2;
        for(hour2=0; hour2<=hour; hour2++)
        {
            //加入小时路径
            char current_type_hour_direct2[1000]= {'\0'};
            char current_type_hour_tmp2[3]= {'\0'};
            two_hour(hour2,current_type_hour_tmp2);
            strcpy(current_type_hour_direct2,current_type_direct2);
            strcat(current_type_hour_direct2,"/");
            strcat(current_type_hour_direct2,current_type_hour_tmp2);
            if(hour2!=hour)
            {
                int minute1;
                for(minute1=0; minute1<60; minute1+=15)
                {
                    char current_type_hour_highrate_direct[1000]= {'\0'};
                    char current_type_hour_tmp3[3]= {'\0'};
                    two_minute(minute1,current_type_hour_tmp3);
                    strcpy(current_type_hour_highrate_direct,current_type_hour_direct2);
                    strcat(current_type_hour_highrate_direct,"/");
                    strcat(current_type_hour_highrate_direct,current_type_hour_tmp3);
                    //路径生成完毕，下面生成文件名
                    char current_file_highratename[20]= {'\0'};
                    creat_highrate_filename(year,day,hour2,minute1,current_type_highrate,current_file_highratename);//生成filename
                    char current_file_highratedirect[1000]= {'\0'};
                    strcpy(current_file_highratedirect,current_type_hour_highrate_direct);
                    strcat(current_file_highratedirect,"/");
                    strcat(current_file_highratedirect,current_file_highratename);
                    //printf("%s\n",current_file_highratedirect);
                    strcpy(download_list[k++],current_file_highratedirect);
                }
            }
            else
            {
                int minute2;
                for(minute2=0; minute2<minute; minute2+=15)
                {
                    char current_type_hour_highrate_direct[1000]= {'\0'};
                    char current_type_hour_tmp3[3]= {'\0'};
                    two_minute(minute2,current_type_hour_tmp3);
                    strcpy(current_type_hour_highrate_direct,current_type_hour_direct2);
                    strcat(current_type_hour_highrate_direct,"/");
                    strcat(current_type_hour_highrate_direct,current_type_hour_tmp3);
                    //路径生成完毕，下面生成文件名
                    char current_file_highratename[20]= {'\0'};
                    creat_highrate_filename(year,day,hour2,minute2,current_type_highrate,current_file_highratename);//生成filename
                    char current_file_highratedirect[1000]= {'\0'};
                    strcpy(current_file_highratedirect,current_type_hour_highrate_direct);
                    strcat(current_file_highratedirect,"/");
                    strcat(current_file_highratedirect,current_file_highratename);
                    //printf("%s\n",current_file_highratedirect);
                    strcpy(download_list[k++],current_file_highratedirect);
                }

            }
        }
    }
    return k;
}

int creat_logfile(int year,int day,int hour,int minute,char download_list[][1000],char undownload_list[][1000])//日志记录，修改日志记录数组，并写入txt文件保存
{
    int i,k=0;//k返回需要下载的文件个数
    int flag=0;
    //创建下载日志路径名称
    char log_name[1000]= {'\0'};
    char file_year[10]= {'\0'};
    char file_day[5]= {'\0'};
    three_day(day ,file_day);
    itoa(year,file_year,10);
    strcpy(log_name,LOGDIRECT);
    strcat(log_name,file_year);
    strcat(log_name,file_day);
    strcat(log_name,".txt");
    //printf("logfilename：%s\n",log_name);
    //printf("log_data_numb=%d\n",log_data_numb);

    FILE *fp=fopen(log_name,"w+");//读写删除模式
    for(i=0; i<log_data_numb; i++)
    {
        flag=Search_undownload_file(download_list[i]);//判断是否存在
        strcpy(log_data[i].local_filename,download_list[i]);
        str_copy(log_data[i].remote_filename,log_data[i].local_filename,14);
        log_data[i].state=flag;
        fprintf(fp,"%s %d\n",log_data[i].local_filename,log_data[i].state);  //写入日志
        if(log_data[i].state==-1)
        {
            strcpy(undownload_list[k++],download_list[i]);
        }
    }
    fclose(fp);
    return k;
}

void get_currentime(MYtime mt)//获取当前时间，并转化成所需要的形式
{
    struct tm *currentime;
    time_t t;
    time(&t);
    currentime=localtime(&t);
    mt.year=currentime->tm_year+1900;
    mt.month=currentime->tm_mon+1;
    mt.day=transfer(mt.year,mt.month,currentime->tm_mday);
    mt.hour=currentime->tm_hour;
    mt.minute=currentime->tm_min;
    mt.second=currentime->tm_sec;
    /*
    if(tmp!=mt.minute)
    {
        printf ( "The current date/time is: %s", asctime (currentime));
        tmp=mt.minute;
    }
    */
}

void creat_hourly_filename(int year,int day,int hour,int type,char hour_filename[])//创建小时文件名(不包含路径)
{
    strcpy(hour_filename,ssss);//加入观测站点
    char tmp1[5]= {'\0'};
    three_day(day,tmp1);
    strcat(hour_filename,tmp1);//加入年内天
    hour_filename[7]=hour+97;
    hour_filename[8]='.';
    char tmp2[5]= {'\0'};
    two_year(year,tmp2);
    hour_filename[9]=tmp2[0];
    hour_filename[10]=tmp2[1];
    hour_filename[11]=t1[type];
    hour_filename[12]='.';
    hour_filename[13]='Z';
    hour_filename[14]='\0';
    //printf("%s\n",hour_filename);
}

void creat_highrate_filename(int year,int day,int hour,int minute,int type,char highrate_filename[])//创建十五分钟文件名(不包含路径)
{
    strcpy(highrate_filename,ssss);
    char tmp1[5]= {'\0'};
    three_day(day,tmp1);
    strcat(highrate_filename,tmp1);//加入年内天
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
    highrate_filename[13]=t2[type];
    highrate_filename[14]='.';
    highrate_filename[15]='Z';
    highrate_filename[16]='\0';
}

void two_year(int year,char y[])//将年的后两位转化为字符
{
    int mode_year;
    mode_year=year%100;
    char tmp[5]= {'\0'};
    itoa(mode_year,tmp,10);
    if(mode_year<10)
    {
        strcpy(y,"0");
        strcat(y,tmp);
    }
    else
    {
        strcpy(y,tmp);
    }
}

void three_day(int day,char d[])//将年内天转化为三字符形式
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

void  two_hour(int hour, char h[])//将小时转化为两字符形式
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

void two_minute(int minute,char m[])//将分钟转化为两字符形式
{
    if(minute==0)
        strcpy(m,"00");
    else
        itoa(minute,m,10);
}

int transfer(int year,int month,int day)//日期转化为年内天
{
    int a[12]= {31,28,31,30,31,30,31,31,30,31,30,31};
    int i,sum=0;
    for(i=0; i<month-1; i++)
    {
        sum+=a[i];
    }
    sum+=day;
    if(year%4==0&&year%100!=0&&month>2) sum++;
    return sum;
}

int Search_undownload_file(char filename[])//查找文件是否存在
{
    int k=0;
    k=access(filename,0);
    //printf("k=%d  %s  exist  %s\n",k,filename,((access(filename,0)==0)?"yes":"no"));
    //return   ((access(filename, 0)==0)?1:0);
    return k;
}
