#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h> //to use read and write
#include <termios.h>
#include <sys/time.h>
#include<stdio.h>


#include "ftp.h"

struct sockaddr_in ftp_server,local_host;
struct hostent * server_hostent;
int socket_control;
int mode=1;//1--PASV 0--PORT

/*
1、FTP服务器路径问题，若是windows，则需要进行转换；默认是linux环境；
2、FTP链接问题，如果没有链接成功，则继续链接三次，每次间隔5秒钟，如果依然连接不成功，则返回网络异常；
3、下载过程中，本地文件以可读可写的形式存在；上传过程中，本地文件以只读的形式存在；
4、如果发出了命令但是没有接受到来自FTP服务器的相应消息，则连续读取三次，如果依然没有消息，则认为是网络异常；
*/


/**
*
* @author: cnscn@163.com
* @reference: lovesnow1314@http://community.csdn.net/Expert/TopicView3.asp?id=5198221
*
* 用新子串newstr替换源字符串src中的前len个字符内所包含的oldstr子串
*
* @param char* dest 目标串，也就是替换后的新串
* @param const char* src 源字符串，被替换的字符串
* @param const char* oldstr 旧的子串，将被替换的子串
* @param const char* newstr 新的子串
* @param int len 将要被替换的前len个字符
*
* @return char* dest 返回新串的地址
*
*/



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


    char filename[] = "mlog.txt";
    FILE *fp;

    if( (fp=fopen( filename ,"at") ) == NULL )
    {
        fp = fopen( filename ,"wt");
    }
    sprintf(msgTemp, "%s\t%s", msgTemp, msg);
    fputs(msgTemp, fp);
    fclose(fp);
}

void debuglog(char * msg)
{
    time_t timer =time(NULL);
    char msgTemp[200] = {0};

    sprintf(msgTemp, "%s",ctime(&timer));
    msgTemp[strlen(msgTemp)-1] = 0;


    char filename[] = "debuglog.txt";
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
	
	error=ftp_get_reply(socket_control);
    if( error!=220 && error!=0)
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
            snprintf(cmd_buf, sizeof(cmd_buf), "PORT %s,%s,%s,%s,%d,%d",
					 ip_1, ip_2, ip_3, ip_4, client_port >> 8, client_port&0xff);

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
				0:ok;
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
	if(strlen(src_file) == 0 || strlen(dst_file) == 0)	
	{
		plog("remote or local file path is invalid.\n");		
		return -1;
	}
	
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
    local_file = open(dst_file, O_RDWR|O_CREAT|O_TRUNC);
    if(local_file < 0)
    {	int i=0;
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
			{
				
				break;
			}                
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
			{
				
				break;
			}                
            else
            {
                write(local_file, rcv_buf, count);
            }
        }
		plog(" get over.\n");

        close(local_file);
        close(get_sock);
    }

	return 0;
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
	usleep(100);
return 0;
}
