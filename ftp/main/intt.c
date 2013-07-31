#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/inotify.h>
#include <malloc.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h> //to use read and write
#include <termios.h>

#include <pthread.h>  

#define DEFAULT_PORT 21

//***********************ftp****************************

struct sockaddr_in ftp_server,local_host;
struct hostent * server_hostent;
int socket_control;
int mode=1;//1--PASV 0--PORT


//***********************multi thread****************************
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;/*初始化互斥锁*/  
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;/*初始化条件变量*/ 

int fileNum = 0; 

//***********************upload watch****************************
struct TaskNode * head;
typedef struct TaskNode//任务信息结构体
{
    char filename[20];//文件名
    char begintime[30];
    char endtime[30];
    int state;//0为创建完，1为上传，2为上传完毕
    struct TaskNode * next;
}TaskNode;





//***********************************************************************

/**
*    function   :   log all information including error;
*    para       :   {char * msg} actural message;
*
*    return     :   {void}
*
*    history    :   {2013.7.18 wujun} frist create;
**/
void plog(char * msg)
{
    time_t timer =time(NULL);
    char msgTemp[200] = {0};

    sprintf(msgTemp, "%s",ctime(&timer));
    msgTemp[strlen(msgTemp)-1] = 0;


    char filename[] = "/home/lu/log.txt";
    FILE *fp;

    if( (fp=fopen( filename ,"at") ) == NULL )
    {
        fp = fopen( filename ,"wt");
    }
    sprintf(msgTemp, "%s\t%s", msgTemp, msg);
    fputs(msgTemp, fp);
    fclose(fp);
}


/**
 *      function    :   fill host ip and port
 *      para        :   {char *host_ip_addr}   like "127.0.0.1"
                        {struct sockaddr_in *host} ftp server add info to being filled that used to connect to ftp server
                        {int port} ftp server port,default 21;
        return      :   {int} error code;
        history     :   {2013.7.18 wujun} fristly be created
**/

int fill_host_addr(char *host_ip_addr,struct sockaddr_in *host,int port)
{
    struct hostent * server_hostent;

    if(port<=0||port>=(1<<16))
        return 254;
    bzero(host,sizeof(struct sockaddr_in));
    host->sin_family=AF_INET;
    host->sin_port=htons(port);
    if(inet_addr(host_ip_addr)!=-1)
    {
        host->sin_addr.s_addr=inet_addr(host_ip_addr);
    }
    else
    {
        if((server_hostent=gethostbyname(host_ip_addr))!=0)
        {
            memcpy(&host->sin_addr,server_hostent->h_addr,sizeof(host->sin_addr));
        }
        else
            return 253;
    }
    return 0;
}



/**
 *      function    :   send command
 *      para        :   {char *s1}  command
                        {char *s2}  parameters after command
                        {int sock_fd}
        return      :   {int} -1:unormal, 0:ok
        history     :   {2013.7.18 wujun} fristly be created
**/

int ftp_send_cmd(const char* s1, const char* s2, int sock_fd)
{
    char send_buf[256];
    int send_err,len;
    if(s1)
    {
        strcpy(send_buf,s1);
        if(s2)
        {
            strcat(send_buf,s2);
            strcat(send_buf,"\r\n");
        }
        else
            strcat(send_buf,"\r\n");

        len=strlen(send_buf);
        send_err=send(sock_fd,send_buf,len,0);

        return 0;
    }

    return -1;
}

/**
 *      function    :   get reply from ftp server;
 *      para        :   {int socket_control} socket file description
 *
 *
 *      return      :   {int} error code, like 0: ok, -1:error ;
 *      history     :   {2013.7.18 wujun} fristly be created
**/
int ftp_get_reply(int sock_fd)
{
    static int reply_code=0,count=0;
    char rcv_buff[512];

    count=read(sock_fd,rcv_buff,510);

    if(count>0)
    {
        reply_code=atoi(rcv_buff);
    }
    else
    {
        plog(" recv no message");
        return 0;
    }

    while(1)
    {
        if (count<=0)
            break;
        rcv_buff[count]='\0';
        count=read(sock_fd,rcv_buff,510);
        plog(rcv_buff);

    }
    return reply_code;
}


/**
 *      function    :   send command 'user' and 'pass' to ftp server;
 *      para        :   {char * user}   ftp user name;
 *                      {char * password}   ftp password;
 *                      {int socket_control} socket file description
 *      return      :   {int} error code, like 0: ok, -1:error ;
 *      history     :   {2013.7.18 wujun} fristly be created
**/
int ftp_login(int socket_control, char * user, char * password)
{
    int err;

    if(ftp_send_cmd("USER ", user, socket_control) < 0)
    {
        plog("Can not send user message.");
        return -1;
    }

    err = ftp_get_reply(socket_control);
    if(err == 331)
    {

        if(ftp_send_cmd("PASS ", password, socket_control) < 0)
        {
            plog("Can not send password message.");
            return -1;
        }
        else
        {
            err = ftp_get_reply(socket_control);
        }

        if(err != 230)
        {
            plog("Password error!");
            return -1;
        }
        return 0;
    }
    else
    {
        plog("User error!");
        return -1;
    }

}


/**
 *      function    :   connect to ftp server;
 *      para        :   {char * ip}   like "127.0.0.1"
 *                      {int port} ftp server port,default 21;
 *                      {char * user} ftp user name;
 *                      {char * password}  ftp password;
 *      return      :   {int} error code;
 *      history     :   {2013.7.18 wujun} fristly be created
**/

int connectFtpServer(char * server_ip, int port, char * user, char * password)
{
    int error;
    char log[500]={0};

    error=fill_host_addr(server_ip,&ftp_server,port);
    if(error==254)
    {
        plog("Invalid port!");
        return -1;
    }

    if(error==253)
    {
        plog("Invalid address!");
        return -1;
    }


    struct timeval outtime;
    int set=0,type=1;
    socket_control=socket(AF_INET,SOCK_STREAM,0);
    if(socket_control<0)
    {
        plog("Creat socket error");
        return -1;
    }

    if(type==1)
    {
        outtime.tv_sec=0;
        outtime.tv_usec=300000;
    }
    else
    {
        outtime.tv_sec=5;
        outtime.tv_usec=0;
    }
    set=setsockopt(socket_control,SOL_SOCKET,SO_RCVTIMEO,&outtime,sizeof(outtime));
    if(set!=0)
    {
        plog("set socket error");
        return -1;
    }

    //connect to the server
    if(connect(socket_control,(struct sockaddr *)&ftp_server,sizeof(struct sockaddr_in))<0)
    {
        memset(log,0,500);
        sprintf(log,"Can't connet to the server:%s,port:%d\n",inet_ntoa(ftp_server.sin_addr),ntohs(ftp_server.sin_port));
        plog(log);
        return -1;
    }
    else
    {
        memset(log,0,500);
        sprintf(log,"Successfully connect to server:%s,port:%d\n",inet_ntoa(ftp_server.sin_addr),ntohs(ftp_server.sin_port));
        plog(log);

    }

    if((error=ftp_get_reply(socket_control))!=220)
    {
        plog("Connect error!");
        return -1;
    }

    while ( (error=ftp_login(socket_control, user, password)) == -1 )
    {
       error=ftp_login(socket_control, user, password);

    }


    return socket_control;

}


int rand_local_port()
{
    int port;
    srand((unsigned)time(NULL));
    port = rand() % 40000 + 1025;
    return port;
}

int xconnect(struct sockaddr_in *s_addr,int type)
{
    struct timeval outtime;
    int set;
    char log[500];
    int s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0)
    {
        plog("Creat socket error");
        return 249;
    }

    if(type==1)
    {
        outtime.tv_sec=0;
        outtime.tv_usec=300000;
    }
    else
    {
        outtime.tv_sec=5;
        outtime.tv_usec=0;
    }
    set=setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&outtime,sizeof(outtime));
    if(set!=0)
    {
        plog("set socket error");
        return -1;
    }

    //connect to the server
    if(connect(s,(struct sockaddr *)s_addr,sizeof(struct sockaddr_in))<0)
    {
        memset(log, 0 ,500);
        sprintf(log,"Can't connet to the server:%s,port:%d\n",inet_ntoa(s_addr->sin_addr),ntohs(ftp_server.sin_port));
        plog(log);
        return 252;

    }
    else
    {
        memset(log, 0 ,500);
        sprintf(log, "Successfully connect to server:%s,port:%d\n",inet_ntoa(s_addr->sin_addr),ntohs(ftp_server.sin_port));
        plog(log);
    }

     return s;

}

int get_port()
{
    char port_respond[512];
    char *temp;
    int count,port_num;
    ftp_send_cmd("PASV",NULL,socket_control);//è¿å¥è¢«å¨æ¨¡å¼
    count=read(socket_control,port_respond,510);
    if(count<=0)
        return 0;
    port_respond[count]='\0';
    if(atoi(port_respond)==227)//ç¡®å®è¿å¥è¢«å¨æ¨¡å¼
    {
        temp=strrchr(port_respond,',');//å¨ä¸²ä¸­æå®å­ç¬¦çæåä¸ä¸ªåºç°ä½ç½®ä»¥æ¾åºn6
        port_num=atoi(temp+1);
        *temp='\0';                   //æªæ­n6æ¥ån5;
        temp=strrchr(port_respond,',');
        port_num+=atoi(temp+1)*256;
        return port_num;
    }
    return 0;
}



/**
 *      function    :   according to mode, set ftp data transfer method
 *      para        :
 *
 *
 *
 *      return      :   {int} error code;
 *      history     :   {2013.7.18 wujun} fristly be created
**/
int xconnect_ftpdata()
{
    if(mode)
    {
        int data_port = get_port();
        if(data_port != 0)
            ftp_server.sin_port=htons(data_port);
        return(xconnect(&ftp_server, 0));
    }
    else
    {
        int client_port, get_sock, opt, set;
        char cmd_buf[32];
        char log[500] = {0};
        struct timeval outtime;
        struct sockaddr_in local;
        char local_ip[24];
        char *ip_1, *ip_2, *ip_3, *ip_4;
        int addr_len =  sizeof(struct sockaddr);
        client_port = rand_local_port();
        get_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(get_sock < 0)
        {
            plog("set work mode PORT and get socket error.");
            return -1;
        }

        //set outtime for the data socket
        outtime.tv_sec = 7;
        outtime.tv_usec = 0;
        opt = SO_REUSEADDR;
        set = setsockopt(get_sock, SOL_SOCKET,SO_RCVTIMEO, &outtime,sizeof(outtime));
        if(set !=0)
        {
            memset(log, 0, 500);
            sprintf(log,"set socket %s errno:%d\n",strerror(errno),errno);
            plog("set socket error");
            return -1;
        }
        set = setsockopt(get_sock, SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));
        if(set !=0)
        {
            memset(log, 0, 500);
            sprintf(log,"set socket %s errno:%d\n",strerror(errno),errno);
            plog(log);
            return -1;
        }

        bzero(&local_host,sizeof(local_host));
        local_host.sin_family = AF_INET;
        local_host.sin_port = htons(client_port);
        local_host.sin_addr.s_addr = htonl(INADDR_ANY);
        bzero(&local, sizeof(struct sockaddr));
        while(1)
        {
            set = bind(get_sock, (struct sockaddr *)&local_host, \
                    sizeof(local_host));
            if(set != 0 && errno == 11)
            {
                client_port = rand_local_port();
                continue;
            }
            set = listen(get_sock, 1);
            if(set != 0 && errno == 11)
            {
                plog("listen()");
                return -1;
            }
            //get local host's ip
            if(getsockname(socket_control,(struct sockaddr*)&local,(socklen_t *)&addr_len) < 0)
                return -1;
            snprintf(local_ip, sizeof(local_ip), inet_ntoa(local.sin_addr));
            //change the format to the PORT command needs.
            local_ip[strlen(local_ip)]='\0';
            ip_1 = local_ip;
            ip_2 = strchr(local_ip, '.');
            *ip_2 = '\0';
            ip_2++;
            ip_3 = strchr(ip_2, '.');
            *ip_3 = '\0';
            ip_3++;
            ip_4 = strchr(ip_3, '.');
            *ip_4 = '\0';
            ip_4++;
            snprintf(cmd_buf, sizeof(cmd_buf), "PORT %s,%s,%s,%s,%d,%d", ip_1, ip_2, ip_3, ip_4, client_port >> 8, client_port&0xff);
            ftp_send_cmd(cmd_buf, NULL, socket_control);
            if(ftp_get_reply(socket_control) != 200)
            {
                plog("Can not use PORT mode!Please use \"mode\" change to PASV mode.\n");
                return -1;
            }
            else
                return get_sock;
        }
    }
}



/**
 *      function    :   download filr from ftp server;
 *      para        :   {char* src_file}   file path in ftp server;
 *                      {char * dst_file} file path in local host;
 *                      {int socket_control} socket file description
 *
 *      return      :   {int}
				-1:create local file error;
				-2:command socket file description error;
				-3:transfer mode set error;
 *      history     :   {2013.7.18 wujun} fristly be created
   			{2013.7.29 wujun} modify return data type from void to int
**/
int ftp_get(char* src_file, char * dst_file, int socket_control)
{
    int get_sock, set, new_sock, i = 0;

    char rcv_buf[512];
    char cover_flag[3];
    struct stat file_info;
    int local_file;
    int count = 0;
/*
    if(!stat(dst_file, &file_info))
    {
        printf("local file %s exists: %d bytes\n", dst_file, (int)file_info.st_size);
        printf("Do you want to cover it? [y/n]");
        fgets(cover_flag, sizeof(cover_flag), stdin);
        fflush(stdin);
        if(cover_flag[0] != 'y')
        {
            printf("get file %s aborted.\n", src_file);
            return;
        }
    }
*/
    local_file = open(dst_file, O_CREAT|O_TRUNC|O_WRONLY);
    if(local_file < 0)
    {	
	plog("creat local file  error!\n");
        return -1;
    }
    get_sock = xconnect_ftpdata();
    if(get_sock < 0)
    {
        plog("socket error!");
        return -2;
    }

    set = sizeof(local_host);

    ftp_send_cmd("TYPE I", NULL, socket_control);
    ftp_get_reply(socket_control);

    ftp_send_cmd("RETR ", src_file, socket_control);
    if(!mode)
    {
        while(i < 3)
        {
            new_sock = accept(get_sock, (struct sockaddr *)&local_host, \
                (socklen_t *)&set);
            if(new_sock == -1)
            {
                plog("accept  errno\n");
                i++;
                continue;
            }
                else break;
        }
        if(new_sock == -1)
        {
            plog("Sorry, you can't use PORT mode. \n");
            return -3;
        }
        ftp_get_reply(socket_control);
        while(1)
        {

            count = read(new_sock, rcv_buf, sizeof(rcv_buf));
            if(count <= 0)
                break;
            else
            {
                write(local_file, rcv_buf, count);
            }
        }
        close(local_file);
        close(get_sock);
        close(new_sock);

    }
    else
    {
        ftp_get_reply(socket_control);
        
        while(1)
        {
            memset(rcv_buf,0,512);
            count = read(get_sock, rcv_buf, 512);

            if(count <= 0)
                break;
            else
            {
                write(local_file, rcv_buf, count);
            }
        }

        close(local_file);
        close(get_sock);
    }
}


/**
 *      function    :   upload filr from ftp server;
 *      para        :   {char* src_file}   file path in local host;
 *                      {char * dst_file} file path in ftp server;
 *                      {int socket_control} socket file description
 *
 *      return      :   {int} 	-1: local file does not exist;
				-2: can not open local file;
				-3: command socket file description error;
				-4: data socket file description error
 *      history     :   {2013.7.18 wujun} fristly be created
			{2013.7.29 wujun} modify return data type from void to int
**/
int ftp_put(char* src_file, char * dst_file, int socket_control)
{

    char send_buff[512];
    struct stat file_info;
    int local_file;
    int file_put_sock,new_sock,count=0,i=0;
    int set=sizeof(local_host);
    char log[500];

    if(stat(src_file,&file_info)<0)
    {
        memset(log,0 ,500);
        sprintf(log,"local file %s doesn't exist!\n",src_file);
        plog(log);
        return -1;
    }
    local_file=open(src_file,O_RDONLY);
    if(local_file<0)
    {
        plog("Open file error");
        return -2;
    }
    file_put_sock=xconnect_ftpdata();
    if(file_put_sock<0)
    {
        ftp_get_reply(socket_control);
        plog("Creat data socket error");
        return -3;
    }
    ftp_send_cmd("STOR ",dst_file,socket_control);
    ftp_get_reply(socket_control);
    ftp_send_cmd("TYPE I",NULL,socket_control);
    ftp_get_reply(socket_control);
    if(mode==0)
    {
        while(i<3)
        {
            new_sock=accept(file_put_sock,(struct sockaddr *)&local_host,(socklen_t*)&set);
            if(new_sock==-1)
            {
                plog("error create new_sock in put port");
                i++;
                continue ;
            }
            else
                break;
        }
        if(new_sock==-1)
        {
            plog("The PORT mode won't work");
            return -4;
        } 
        while(1)
        {
            count=read(local_file,send_buff,sizeof(send_buff));
            if(count<=0)
                break;
            else
            {
                write(new_sock,send_buff,sizeof(send_buff));
            }
            close(local_file);
            close(file_put_sock);
            close(new_sock);
        }
    }
    if(mode==1)
    {
        while(1)
        {
            count=read(local_file,send_buff,sizeof(send_buff));
            if(count<=0)
                break;
            else
            {
                write(file_put_sock,send_buff,count);
            }
        }
	
        close(local_file);
        close(file_put_sock);
    }
return 0;
}
//***********************************************************************

void display()
{
TaskNode *p;
p=head;
while(p!=NULL)
{
 printf("链表：%s\t%s\t%d\n",p->filename,p->begintime,p->state);
 p=p->next;
}
}


void insertl(TaskNode * p0)
{

    if(head==NULL)
    {
            head=p0;
            p0->next=NULL;
    }
    else
    {
 p0->next=head;
        head=p0;
       
    }
display();
}

void search(char * name)
{

	TaskNode *p1,*p2;
	if(head==NULL)  //为空
	{
	   printf("文件未创建\n");
	}

	p1=head;

	while(strcmp(name,p1->filename)!=0 && p1->next!=NULL)
	{
		p2=p1;
		p1=p1->next;
	}

	if(strcmp(name,p1->filename)==0)
	{
		if(p1->state==0)
		{
            if(p1==head)
                head=p1->next;
            else
               p2->next=p1->next;
            free(p1);
            printf("文件上传成功\n");
		}
		else if(p1->state==2)
            printf("state=2\n");//补全，待定

	}
	else printf("元素找不到\n");//待定
}


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

void write_log(TaskNode * p){
struct tm *t;
char * curtime;
int k;
t=gettime();
curtime=getchartime();

int year=t->tm_year+1900;
int month=t->tm_mon+1;
char name[5]=".xml";
char logpath[12];
sprintf(logpath,"%04d%02d%s",year,month,name);

FILE *fp;
fp=fopen(logpath,"a+");

fprintf(fp,"upload\t%s\t%s\r",p->filename,p->begintime);
fclose(fp);//没有那个文件或目录

}


static void _inotify_event_handler(struct inotify_event *event)
{

 	if(event->mask & IN_CLOSE_WRITE)
	{

		printf("IN_CLOSE_WRITE\n");
		printf("event->name: %s\n", event->name);

		TaskNode *p0;
		p0=(TaskNode *)malloc(sizeof(TaskNode));
		strcpy(p0->filename,event->name);
		strcpy(p0->begintime,getchartime());
		//printf("传入：%s\n",p0->filename);
		p0->state=0;//创建完

		pthread_mutex_lock(&mutex);/*锁住互斥量*/  

		insertl(p0);
sleep(3);
		fileNum ++;
		printf("new %d\n",fileNum);
		
 		pthread_cond_signal(&cond);/*条件改变，发送信号，通知upload进程*/  
		
		pthread_mutex_unlock(&mutex);/*解锁互斥量*/  


		//write_log(p0);
	}	

}



void watch()
{

	unsigned char buf[1024] = {0};
	struct inotify_event *event = {0};
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, "./src",IN_CLOSE_WRITE);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	while(1)
	{
		if (select(fd + 1, &fds, NULL, NULL, NULL) > 0) 
		{
			int len, index = 0;
			while (((len = read(fd, &buf, sizeof(buf))) < 0) && (errno == EINTR));
			while (index < len) 
			{
				event = (struct inotify_event *)(buf + index);
				_inotify_event_handler(event);
				index += sizeof(struct inotify_event) + event->len;
			}
		}
		
		
	}
	inotify_rm_watch(fd, wd);

}


int upload()
{	
	int i,j;
	
	while(1)
	{
		pthread_mutex_lock(&mutex);/*锁住互斥量*/  
		if(fileNum < 1)
		{
			pthread_cond_wait(&cond,&mutex);/*解锁mutex，并等待cond改变*/  
		}
		fileNum --;
		
		if ( connectFtpServer("192.168.0.222", 21, "public", "123456") > 0 )
		{

/*RIGHT		
ftp_put("src/1.txt", "upload/3.txt", socket_control);
		    //ftp_put("erxt", "2.txt", socket_control);
			printf("upload success.");
		    	close(socket_control);
*/
///*
		TaskNode * p1;
		p1=head;
		//printf("fliename:%s",p1->filename);
		
		int ftperror = 0;
		while( p1 != NULL && p1->state == 0 )
		{
			char oriadd[8]="./src/";
			char destadd[10]="upload/";
			char ori[50],dest[50];
			//printf("fliename:%s",p1->filename);
			sprintf(ori,"%s%s",oriadd,p1->filename);
			sprintf(dest,"%s%s",destadd,p1->filename);
			printf("%s\n%s\n",ori,dest);	
			

			if( (ftperror = ftp_put(ori, dest, socket_control) ) == 0)
			{
				//sleep(1);
				printf("%s----upload success.\n",p1->filename);
				close(socket_control);
				p1->state=1;
	
				display();
			}
			else printf("error\n");
			printf("fpterror:%d\n",ftperror);
			p1=p1->next;
			ftperror = -99;	
	}	 	
	
	
//*/

		}

		else
		    printf("%d\n", socket_control);

		pthread_mutex_unlock(&mutex);/*解锁互斥量*/ 
		
	}	
 
}

int main(void)  
{  
    pthread_t _watch;  
    pthread_t _upload;  
	//while(1)
	{
		pthread_create(&_watch,NULL,watch,(void *)NULL);/*创建进程watch*/  
		pthread_create(&_upload,NULL,upload,(void *)NULL); /*创建进程upload*/  
		//pthread_join(watch, NULL);/*等待进程watch结束*/  
		//pthread_join(upload, NULL);/*等待进程upload结束*/  
	}
	while(1);

    //pthread_mutex_destroy(&mutex);  
    //pthread_cond_destroy(&cond);  
    exit(0);  
} 
