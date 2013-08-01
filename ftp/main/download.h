#ifndef _DOWNLOAD_H_
#define _DOWNLOAD_H_


/*----------------------------------downlad --------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/	

#define	  DOWNLOAD_FILE_NONEXIST			-1
#define   DOWNLOAD_FILE_EXIST				1
#define   DOWNLOAD_FILE_DOWNLOAD_FAILED	    2

#define   MAX_DOWNLOAD_TASK_NUM	1000    //daily task number's max size 
#define	  MAX_FTP_FILE_PATH_LEN	1000	//remote or local file name's max size
#define   MAX                   1000    //The max size of locale array
#define   R_year                366        //Days of leap year
#define   P_year                365        //Days of leap year
#define   Daily                 "/daily"
#define   Hourly                "/hourly"
#define   Highrate              "/highrate"


typedef  struct
{
    char remote_filename[MAX_FTP_FILE_PATH_LEN];
    char local_filename [MAX_FTP_FILE_PATH_LEN];
    int  state;        //-1(exist)&&1(non_exist)
} DownloadList;


typedef struct        //自定义时间数据结构
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} MYtime;

void module_control();                                                //Ä£¿éµ÷¶È
void get_currentime(MYtime *mt);                                    //»ñÈ¡µ±Ç°Ê±Œä
int  Search_undownload_file(char filename[]);                      //²éÕÒÎÄŒþÊÇ·ñŽæÔÚ
int  transfer(int year,int month,int day);                        //ÈÕÆÚ×ª»¯ÎªÄêÄÚÌì
void two_year(int year,char y[]);                                //œ«ÄêµÄºóÁœÎ»×ª»¯Îª×Ö·û
void three_day(int day,char d[]);                               //œ«Ìì×ª»¯ÎªÈý×Ö·ûÐÎÊœ
void two_hour(int hour, char h[]);                             //œ«Ð¡Ê±×ª»¯ÎªÁœ×Ö·ûÐÎÊœ
void two_minute(int minute,char m[]);                         //œ«·ÖÖÓ×ª»¯ÎªÁœ×Ö·ûÐÎÊœ
void creat_hourly_filename(int year,int day,int hour,int type, char hour_filename[]);//ŽŽœšÐ¡Ê±ÎÄŒþÃû
void creat_highrate_filename(int year,int day,int hour,int minite,int type,char highrate_filename[]);//ŽŽœšÊ®Îå·ÖÖÓÎÄŒþÃû
int  creat_current_direct(int year,int day,int hour,int minute,char download_list[][MAX]);//ŽŽœšµ±Ç°Ê±¿ÌÏÂÔØÁÐ±í(current)
int  creat_logfile(int year,int day,int hour,int minute,char download_list[][MAX],int tmp);//ÈÕÖŸŒÇÂŒ
void str_copy(char A[],char B[],int i);//°ÑBŽÓµÚižö×Ö·ûŒÓµœ¡±igmas¡°ºóžŽÖÆžøA;
int itoa(int tempInt, char * str, int dec); //int to char*
#endif
