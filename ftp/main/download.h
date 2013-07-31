
#define   MAX   1000
#define   ROOTDIRECT      "/home/jung/tmp/FromDC/"           //žùÄ¿ÂŒ
#define   LOGDIRECT       "/home/jung/tmp/FromDC/logs/"     //ÈÕÖŸÎÄŒþ±£ŽæÂ·Ÿ¶£¬ŒÇÂŒµ±ÌìÐèÒªÏÂÔØµÄÈ«²¿ÎÄŒþŒÇÂŒ
#define   Daily           "/daily"
#define   Hourly          "/hourly"
#define   Highrate        "/highrate"
#define   R_year          6
#define   P_year          5
#define   ssss            "ssss"
#define   IGMAS           "download/iGMAS/"

char   t1[13]= {'D','N','G','L','R','M','T','J','A','K','I','E'};
char   t2[7]=  {'D','N','G','L','R','M'};
int    numb1=12;
int    numb2=6;
int tmp=-1;

typedef  struct
{
    char remote_filename[1000];
    char local_filename[1000];
    //char begin_time[];//Žý¶š×Ö¶Î
    //char end_time[];
    int  state;        //-1(²»ŽæÔÚ)ºÍ1(ŽæÔÚ)
} Logdata;
Logdata log_data[1000]; //È«ŸÖ±äÁ¿£¬·µ»ØÃ¿ÌõÊýŸÝµÄÃû³Æ£šŽøÂ·Ÿ¶£©ÒÔŒ°×ŽÌ¬
int     log_data_numb; //ŒÇÂŒµ±ÌìŒÇÂŒµÄÌõÊý

typedef struct        //×Ô¶šÒåÊ±ŒäÊýŸÝœá¹¹
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} MYtime;



int  transfer(int year,int month,int day);                        //ÈÕÆÚ×ª»¯ÎªÄêÄÚÌì
void two_year(int year,char y[]);                                //œ«ÄêµÄºóÁœÎ»×ª»¯Îª×Ö·û
void three_day(int day,char d[]);                               //œ«Ìì×ª»¯ÎªÈý×Ö·ûÐÎÊœ
void two_hour(int hour, char h[]);                             //œ«Ð¡Ê±×ª»¯ÎªÁœ×Ö·ûÐÎÊœ
void two_minute(int minute,char m[]);                         //œ«·ÖÖÓ×ª»¯ÎªÁœ×Ö·ûÐÎÊœ
void creat_hourly_filename(int year,int day,int hour,int type, char hour_filename[]);                 //ŽŽœšÐ¡Ê±ÎÄŒþÃû
void creat_highrate_filename(int year,int day,int hour,int minite,int type,char highrate_filename[]);//ŽŽœšÊ®Îå·ÖÖÓÎÄŒþÃû
void get_currentime(MYtime mt);                            //»ñÈ¡µ±Ç°Ê±Œä£¬²¢×ª»¯³ÉËùÐèÒªµÄÐÎÊœ
int  Search_undownload_file(char filename[]);              //²éÕÒÎÄŒþÊÇ·ñŽæÔÚ
void module_control();                                    //Ä£¿éµ÷¶È
int  creat_current_direct(int year,int day,int hour,int minute,char download_list[][1000]);//ŽŽœšµ±Ç°Ê±¿ÌÏÂÔØÁÐ±í(00:00¡ª¡ªcurrent)
int  creat_logfile(int year,int day,int hour,int minute,char download_list[][1000],char undownlist_list[][1000]);//ÈÕÖŸŒÇÂŒ
//void creat_day_direct(int year,int day,char log_filename[]);      //ŽŽœšÌìÏÂÔØÁÐ±í±£ŽæµœÈÕÖŸtxt
int itoa(int tempInt, char * str, int dec);

