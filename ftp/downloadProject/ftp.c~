#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h> //to use read and write
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#include "ftp.h"

struct sockaddr_in ftp_server,local_host;
struct hostent * server_hostent;
int mode=1;//1--PASV 0--PORT



/*************************************************************************************************
*    function   :   check out whether one path does exist.
*    para       :   {char *filePath} the specified path.
*
*    return     :   {int } 0: ok; -1:error.
*
*    history    :   {2013.8.5 wujun} frist create;
**************************************************************************************************/
int dirIsExist(char *filePath)
{
    if( filePath == NULL )
    {
        plog("%s\n","Fun(dirIsExist): parameter is NULL.\n");
        return -1;
    }

    int len =  strlen(filePath);
    if( len == 0)
    {
        plog("%s\n","Fun(dirIsExist): parameter content is '0' .\n");
        return -1;
    }

    char *path;

    path = (char *)malloc(len+1);
    if( path == NULL)
    {
        plog("%s\n","Fun(dirIsExist): malloc \"path\" error.\n");
        return -1;
    }

    memset(path,0,len+1);
    strcpy(path, filePath);

    int p;
    for( p = 1; p <= len; p++ )
    {
        if((path[p] == '/') || (path[p] == 0) )
        {
            path[p] = 0;
            if( access(path, 0) != 0)
            {
                if( mkdir(path, 0777) == -1 )
                {
                    plog("Fun(dirIsExist): mkdir %s error.\n.", path);
                    return -1;
                }
            }

             path[p] = '/';
        }
    }

    return 0;
}


/**************************************************************************************************
*    function   :   delay some time
*    para       :   {void}
*
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**************************************************************************************************/

void ftp_delay()
{
    int b=0,e=0;
    for(b=0;b<2000;b++)
    for(e=0;e<2000;e++);
}


/**************************************************************************************************
*    function   :   log all information including error;
*    para       :   {char * msg} actural message;
*
*    return     :   {void}
*
*    history    :   {2013.7.18 wujun} frist create;
**************************************************************************************************/

int plog(char *format,...)
{
   	if( format == NULL)
   	{
        printf("Fun(plog): parameter is Null.\n");
        return -1;
   	}

    char startTime[100] ;
	memset(startTime, 0, 100);

   	struct tm 	*st;
	time_t 		t;

    t = time(NULL);
    st = localtime(&t);//get current datetime

    int day = st->tm_mday;
    int month = st->tm_mon + 1;
    int year = st->tm_year + 1900;
    sprintf(startTime,"%d-%d-%d %d:%d:%d", year, month, day, st->tm_hour, st->tm_min, st->tm_sec);

    char logFile[100] ;
    memset(logFile, 0, 100);

    sprintf(logFile, "log/ftp/%04d%02d%02d.log", year, month, day);
    dirIsExist("log/ftp/");

    FILE *dwLogFp;
    if( (dwLogFp=fopen( logFile ,"a+") ) == NULL )
    {
        printf("%s\n","Fun(plog):create file  error.\n ");
        return -1;
    }

    va_list arg_ptr;
    va_start(arg_ptr, format);
    fprintf(dwLogFp,"%s\t",startTime);
    vfprintf(dwLogFp, format, arg_ptr);
    va_end(arg_ptr);

    fclose(dwLogFp);
    ftp_delay();

}





/**************************************************************************************************
 *      function    :   fill host ip and port
 *      para        :   {char *host_ip_addr}   like "127.0.0.1"
                        {struct sockaddr_in *host} ftp server add info to being filled that used to connect to ftp server
                        {int port} ftp server port,default 21;
        return      :   {int} error code;
        history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/

int fill_host_addr(char *host_ip_addr,struct sockaddr_in *host,int port)
{
    struct hostent * server_hostent;

    if(port<=0||port>=(1<<16))
    {
        return 254;
    }

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
        {
            return 253;
        }

    }
    return 0;
}



/**************************************************************************************************
 *      function    :   send command
 *      para        :   {char *s1}  command
                        {char *s2}  parameters after command
                        {int sock_fd}
 *      return      :   {int} -1:unormal, 0:ok
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/

int ftp_send_cmd(const char* s1, const char* s2, int sock_fd)
{
    char send_buf[256];
    memset(send_buf, 0, 256);
    int send_err = 0,len;
    if(s1)//si is not null
    {
        strcpy(send_buf,s1);
        if(s2)//s2 is not null;
        {
            strcat(send_buf,s2);
            strcat(send_buf,"\r\n");
        }
        else
        {
            strcat(send_buf,"\r\n");
        }

        len=strlen(send_buf);
        send_err=send(sock_fd,send_buf,len,0);
        if(send_err == len)
        {
            return 0;//return ok;
        }
        else
        {
            plog("%s","Fun(ftp_send_cmd): send error.\n");
            return -1;//return error
        }


    }

    plog("%s","Fun(ftp_send_cmd): parameter s1 is null.\n");
    return -1;//retrun error
}

/**************************************************************************************************
 *      function    :   get reply from ftp server;
 *      para        :   {int sock_fd} socket file description
 *
 *
 *      return      :   {int} error code, like 0: ok, -1:error ;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int ftp_get_reply(int sock_fd, char* buff )
{

    int reply_code=0,count=0;
    char rcv_buff[2048];
    memset( rcv_buff, 0, 2048);
    struct timeval timeout = {20,0};
    int ret;
    fd_set rfds;

    FD_ZERO(&rfds);  /* 清空集合 */
    FD_SET(sock_fd, &rfds);  /* 将fp添加到集合，后面的FD_ISSET和FD_SET没有必然关系，这里是添加检测 */

    ret=select(sock_fd+1, &rfds, NULL, NULL, &timeout);

    if(0 > ret)
    {
        return -1;
    }
    else if(0 == ret)
    {
        return -1;
    }
    else
    {
        if(FD_ISSET(sock_fd,&rfds))  /* 这里检测的是fp在集合中是否状态变化，即可以操作。 */
        {
            memset (rcv_buff,0,2048);
            ftp_delay();
            count = recv(sock_fd, rcv_buff, 2047, 0);
            if(buff != NULL) strcpy(buff,rcv_buff);
            if(0 == count)
            {
                plog("%s\n","Fun(ftp_get_reply):recv no message.");
                return -1;    /* 此处需要检测！否则ftp发送数据时，后面会循环接收到0字节数据 */
            }else
            {
                reply_code=atoi(rcv_buff);
                plog("%s", rcv_buff);

            }
        }
    }

    return reply_code;
}


/**************************************************************************************************
 *      function    :   send command 'user' and 'pass' to ftp server;
 *      para        :   {char * user}   ftp user name;
 *                      {char * password}   ftp password;
 *                      {int socket_control} socket file description
 *      return      :   {int} error code, like 0: ok, -1:error ;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int ftp_login(int socket_control, char * user, char * password)
{
    int err;

    if(ftp_send_cmd("USER ", user, socket_control) < 0)
    {
        plog("%s", "Fun(ftp_login): send command 'USER' error.\n");
        return -1;
    }

    err = ftp_get_reply(socket_control, NULL);
    if(err == 331)
    {

        if(ftp_send_cmd("PASS ", password, socket_control) < 0)
        {
            plog("%s", "Fun(ftp_login):send command 'PASS' error.\n");
            return -1;
        }

        err = ftp_get_reply(socket_control, NULL);


        if(err != 230)
        {
            plog("%s", "Fun(ftp_login): Password error!\n");
            return -1;
        }
        else
        {
            plog("%s", "Fun(ftp_login): Password correct!\n");
            return 0;
        }

    }
    else if ( err == 530)
    {
        plog("%s", "Fun(ftp_login): Already connected.\n");
        return -1;
    }
    else
    {
        plog("%s", "Fun(ftp_login): other error.\n");
        return -1;
    }
}



/**************************************************************************************************
 *      function    :   connect to ftp server;
 *      para        :   {char * ip}   like "127.0.0.1"
 *                      {int port} ftp server port,default 21;
 *                      {char * user} ftp user name;
 *                      {char * password}  ftp password;
 *      return      :   {int} error code;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/

int connectFtpServer(char * server_ip, int port, char * user, char * password)
{
    int socket_control;
    
    if( server_ip == NULL || user == NULL)
    {
        plog( "%s\n\n", "Fun(connectFtpServer): server_ip or user is NULL." );
        return -1;
    }

    int error;
    char log[1000]={0};

    error = fill_host_addr( server_ip, &ftp_server, port );
    if( error == 254 )
    {
        plog("%s\n\n", "Fun(connectFtpServer): Invalid port!");
        return -1;
    }

    if( error == 253 )
    {
        plog("%s\n\n", "Fun(connectFtpServer): Invalid address!");
        return -1;
    }

    struct timeval outtime;
    int type = 1;
    socket_control = socket( AF_INET,SOCK_STREAM, 0 );
    if( socket_control < 0 )
    {
        plog("%s\n\n", "Fun(connectFtpServer): Creat socket error.");
        return -1;
    }

    if( type == 1 )
    {
        outtime.tv_sec = 10;
        outtime.tv_usec = 0;// 300000;
    }
    else
    {
        outtime.tv_sec = 5;
        outtime.tv_usec = 0;
    }


    //connect to the server
    if(connect( socket_control, (struct sockaddr *) &ftp_server, sizeof(struct sockaddr_in) ) <0 )
    {
        memset( log, 0, 1000 );
        sprintf(log,"Fun(connectFtpServer): Can't connet to the server:%s,port:%d\n\n",inet_ntoa(ftp_server.sin_addr),ntohs(ftp_server.sin_port));
        plog( "%s", log );
        close(socket_control);
        return -1;
    }
    else
    {
        memset( log, 0, 1000 );

        sprintf( log, "Fun(connectFtpServer): Successfully connect to server:%s,port:%d\n",inet_ntoa(ftp_server.sin_addr),ntohs(ftp_server.sin_port));
        plog( "%s", log );
    }
	error = ftp_get_reply( socket_control, NULL );
	if( error == 421 )//There are too many connections from your internet address.
	{
        plog("%s\n\n","Fun(connectFtpServer): 421 too many connections.");
        close(socket_control);
        return -1;
	}
	else if( error == 220)//Successfully connect
	{
        if( (error = ftp_login(socket_control, user, password)) == -1 )
        {
            plog("%s\n\n", "Fun(connectFtpServer): user or password error.");
            close(socket_control);
            return -1;
        }

        plog("Fun(connectFtpServer): socket_control = %d\n",socket_control);
        return socket_control;
	}
	else
	{
        plog("%s\n\n", "Fun(connectFtpServer): Connect error!");
        close(socket_control);
        return -1;
	}

}

/**************************************************************************************************
 *      function    :   select a random port
 *      para        :   {void}
 *      return      :   {int} random port;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int rand_local_port()
{
    int port;
    srand( (unsigned) time(NULL) );
    port = rand() % 40000 + 1025;

    return port;
}

/**************************************************************************************************
 *      function    :   socket connect.
 *      para        :   {struct sockaddr_in *s_addr} address.
                        {int type} connect type
 *      return      :   {int} socket fd;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int xconnect(struct sockaddr_in *s_addr,int type)
{
    struct timeval outtime;
    int set;
    char log[500];

    if( s_addr == NULL )
    {
        plog("%s", "Fun(xconnect): s_addr is NULL.\n");
        return -1;
    }

    int s = socket( AF_INET, SOCK_STREAM, 0 );
    if( s < 0 )
    {
        plog("%s", "Fun(xconnect): Creat socket error.\n");
        return -1;
    }

    if(type==1)
    {
        outtime.tv_sec = 1;
        outtime.tv_usec = 0;
    }
    else
    {
        outtime.tv_sec=5;
        outtime.tv_usec=0;
    }
    set = setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, &outtime, sizeof(outtime) );

    if( set != 0 )
    {
        plog("%s", "Fun(xconnect): set socket error.\n");
        close(s);
        return -1;
    }

    //connect to the server
    int sockfd = -1;
    sockfd = connect( s, (struct sockaddr *)s_addr, sizeof(struct sockaddr_in) );
    if( sockfd < 0 )
    {
        memset(log, 0 ,500);
        sprintf(log,"Fun(xconnect): Can't connet to the server:%s,port:%d\n",inet_ntoa(s_addr->sin_addr),ntohs(ftp_server.sin_port));
        plog("%s", log);
        close(s);
        return -1;
    }
    else
    {
        memset(log, 0 ,500);
        sprintf(log, "Fun(xconnect): Successfully connect to server:%s,port:%d\n",inet_ntoa(s_addr->sin_addr),ntohs(ftp_server.sin_port));
        plog("%s", log);
    }

    return s;
}

/**************************************************************************************************
 *      function    :   get ftp port, when mode=1, port =21
 *      para        :   {void} .

 *      return      :   {int } port
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int get_port(int socket_control)
{
    char port_respond[2048];
    char *temp;
    int count,port_num;

    int error = ftp_send_cmd("PASV",NULL,socket_control);
    if(  error < 0)
    {
        plog("%s","Fun(get_port) : Send 'PASV' command error.\n");
        return 0;//è¿å¥è¢«å¨æ¨¡å¼
    }

	memset(port_respond, 0, 2048);
    //count = recv(socket_control,port_respond,510, MSG_WAITALL);
    count =  ftp_get_reply(socket_control, port_respond);
    if( count <= 0 )
    {
        plog("%s", "Fun(get_port): recv no message or other error.\n");
        return 0;
    }

   	plog("Fun(get_port): %s.", port_respond);
    if( atoi( port_respond ) == 227 )//ç¡®å®è¿å¥è¢«å¨æ¨¡å¼
    {
        temp = strrchr( port_respond, ',' );//å¨ä¸²ä¸­æå®å­ç¬¦çæåä¸ä¸ªåºç°ä½ç½®ä»¥æ¾åºn6
        port_num = atoi( temp+1 );
        *temp = '\0';                   //æªæ­n6æ¥ån5;
        temp = strrchr( port_respond, ',');
        port_num += atoi( temp+1 ) * 256;
        return port_num;
    }

    return 0;
}



/**************************************************************************************************
 *      function    :   according to mode, set ftp data transfer method
 *      para        :
 *      return      :   {int} error code;
 *      history     :   {2013.7.18 wujun} fristly be created
**************************************************************************************************/
int xconnect_ftpdata(int socket_control)
{
    if(mode)
    {
        int data_port = get_port(socket_control);
        if( data_port != 0 )
        {
            ftp_server.sin_port=htons(data_port);
        }

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
            plog("%s", "Fun(xconnect_ftpdata):set work mode PORT and get socket error.\n");
            return -1;
        }

        //set outtime for the data socket
        outtime.tv_sec = 1;
        outtime.tv_usec = 0;
        opt = SO_REUSEADDR;
        //set = setsockopt(get_sock, SOL_SOCKET,SO_RCVTIMEO, &outtime,sizeof(outtime));
        if(set !=0)
        {
            memset(log, 0, 500);
            sprintf(log,"Fun(xconnect_ftpdata):set socket %s errno:%d\n",strerror(errno),errno);
            plog("%s", log);
            close(get_sock);
            return -1;
        }
        set = setsockopt(get_sock, SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));
        if(set !=0)
        {
            memset(log, 0, 500);
            sprintf(log,"Fun(xconnect_ftpdata):set socket %s errno:%d\n",strerror(errno),errno);
            plog("%s",log);
            close(get_sock);
            return -1;
        }

        bzero(&local_host,sizeof(local_host));
        local_host.sin_family = AF_INET;
        local_host.sin_port = htons(client_port);
        local_host.sin_addr.s_addr = htonl(INADDR_ANY);
        bzero(&local, sizeof(struct sockaddr));
        while(1)
        {
            set = bind(get_sock, (struct sockaddr *)&local_host, sizeof(local_host));
            if(set != 0 && errno == 11)
            {
                client_port = rand_local_port();
                continue;
            }
            set = listen(get_sock, 1);
            if(set != 0 && errno == 11)
            {
                plog("%s","Fun(xconnect_ftpdata):listen().\n");
                close(get_sock);
                return -1;
            }
            //get local host's ip
            if(getsockname(socket_control,(struct sockaddr*)&local,(socklen_t *)&addr_len) < 0)
            {
                plog("%s", "Fun(xconnect_ftpdata): get sock name error.\n");
                close(get_sock);
                return -1;
            }

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

            if( ftp_send_cmd( cmd_buf, NULL, socket_control ) < 0 )
            {
                plog("%s", "Fun(xconnect_ftpdata):send 'PORT' command error.\n");
                close(get_sock);
                return -1;
            }

            if(ftp_get_reply(socket_control, NULL) != 200)
            {
                plog("%s","Fun(xconnect_ftpdata):Can not use PORT mode!Please use \"mode\" change to PASV mode.\n");
                close(get_sock);
                return -1;
            }
            else
            {
                return get_sock;
            }

        }
    }
}


/**************************************************************************************************
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
**************************************************************************************************/
int ftp_get(char* src_file, char * dst_file, int socket_control)
{
    int get_sock, set, new_sock, i = 0;
    char rcv_buf[512];
    char cover_flag[3];
    struct stat file_info;
    FILE * local_file;
    int count = 0;

    int replayId = 0;

    if( src_file == NULL || dst_file == NULL)
    {
        plog("%s","Fun(ftp_get) : file path is invalid.\n\n");
		return DOWNLOAD_LOCAL_FILENAME_NULL;
    }

    get_sock = xconnect_ftpdata(socket_control);
    if(get_sock < 0)
    {
        plog("%s","Fun(ftp_get) :socket error!\n\n");
        return DOWNLOAD_CONNECT_SOCKET_ERROR;
    }

    set = sizeof(local_host);

    if( ftp_send_cmd("TYPE I", NULL, socket_control) < 0 )
    {
        plog("%s", "Fun(ftp_get) :send 'TYPE I' command error.\n\n");
        close(get_sock);
        return -1;
    }
    if( ftp_get_reply(socket_control, NULL) <= 0 )
    {
        plog("%s", "Fun(ftp_get) :'TYPE I' recv no message.\n");
        close(get_sock);
        return -1;
    }

    if( ftp_send_cmd("RETR ", src_file, socket_control) < 0)
    {
        plog("%s", "Fun(ftp_get) :send 'RETR' command error.\n\n");
        close(get_sock);
        return -1;
    }

    if(!mode)
    {
        while(i < 3)
        {
            new_sock = accept(get_sock, (struct sockaddr *)&local_host, (socklen_t *)&set);
            if(new_sock == -1)
            {
                plog("%s","Fun(ftp_get) :accept  errno.\n");
                i++;
                continue;
            }
            else
            {
                break;
            }
        }
        if( new_sock == -1 )
        {
            plog("%s","Fun(ftp_get) :Sorry, you can't use PORT mode. \n\n");
            close(get_sock);
            return DOWNLOAD_PORT_MODE_ERROR;
        }

        if( ( replayId = ftp_get_reply(socket_control, NULL ) ) <= 0 )
        {
            plog("%s", "Fun(ftp_get) :recv no message or network error.\n\n");
            close(get_sock);
            return -1;
        }

        if(replayId == 550)//remote file does not exist.
        {
            plog("%s%s", src_file, " does not exsit.\n");
            close(get_sock);
            return DOWNLOAD_REMOTE_FILE_NOEXIST;
        }

        local_file = fopen(dst_file, "w+");
        if( local_file == NULL )
        {
            plog("%s","Fun(ftp_get) :creat local file  error!\n\n");
            close(get_sock);
            return DOWNLOAD_CREAET_LOCALFILE_ERROR;
        }

        while(1)
        {

            count = recv(new_sock, rcv_buf, sizeof(rcv_buf),MSG_WAITALL);
            if(count <= 0)
			{
				fclose(local_file);
                close(get_sock);
                close(new_sock);

                char commmand[1024];
                sprintf(commmand, "chmod 0777 %s", dst_file);
                system(commmand);

                ftp_delay();
				plog("%s\t%s",src_file," download successfully.\n\n\n");
				return FTP_DOWNLOAD_SUCCESS;
			}
            else
            {
                fprintf(local_file, "%s",rcv_buf);
            }
        }
    }
    else
    {
        if( ( replayId=ftp_get_reply(socket_control, NULL) ) <= 0 )
        {
            plog("%s", "Fun(ftp_get) :'RETR' recv no message.\n\n");
            close(get_sock);
            return -1;
        }

        if(replayId == 550)//remote file does not exist.
        {
            plog("%s%s", src_file, " does not exsit.\n");
            close(get_sock);
            return DOWNLOAD_REMOTE_FILE_NOEXIST;
        }

        local_file = fopen(dst_file, "w+");
        if( local_file == NULL )
        {
            plog("%s","Fun(ftp_get) :creat local file  error!\n\n");
            close(get_sock);
            return DOWNLOAD_CREAET_LOCALFILE_ERROR;
        }

        while(1)
        {
            memset(rcv_buf,0,512);
            count = recv(get_sock, rcv_buf, 512, MSG_WAITALL);

            if(count <= 0)
			{
                fclose(local_file);
                close(get_sock);

                char commmand[1024];
                sprintf(commmand, "chmod 0777 %s", dst_file);
                system(commmand);

                ftp_delay();
				plog("%s\t%s",src_file," download successfully.\n\n");
                return FTP_DOWNLOAD_SUCCESS;
			}
            else
            {                
				fwrite (rcv_buf , sizeof(char), sizeof(rcv_buf), local_file);
            }
        }
    }
}



/**************************************************************************************************
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
**************************************************************************************************/
int ftp_put(char* src_file, char * dst_file, int socket_control)
{

    char send_buff[512];
    struct stat file_info;
    int local_file;
    int file_put_sock,new_sock,count=0,i=0;
    int set=sizeof(local_host);
    char log[500];

    if( stat( src_file, &file_info ) < 0 )
    {
        memset( log, 0, 500 );
        sprintf( log, "Fun(ftp_put):upload:local file %s doesn't exist!\n\n", src_file );
        plog( "%s", log );

        return UPLOAD_LOCAL_FILENAME_NULL;
    }

    local_file = fopen( src_file, "r" );
    if( local_file < 0 )
    {
        plog("%s","Fun(ftp_put): Open file error.\n\n");
        return UPLOAD_LOCAL_OPEN_ERROR;
    }

    file_put_sock = xconnect_ftpdata( socket_control);
    if( file_put_sock < 0 )
    {
        ftp_get_reply( socket_control, NULL );
        plog("%s", "Fun(ftp_put):Creat data socket error.\n\n");
        return UPLOAD_DATA_SOCKET_ERROR;
    }

    if( ftp_send_cmd("STOR " , dst_file,socket_control ) < 0 )
    {
        plog("%s", "Fun(ftp_put):send 'STOR' command error.\n\n");
        close(file_put_sock);
        return -1;
    }
    if( ftp_get_reply(socket_control, NULL) == 0)
    {
        plog("%s", "Fun(ftp_put):recv no message after sending 'STOR' command.\n\n");
        close(file_put_sock);
        return -1;
    }

    if( ftp_send_cmd("TYPE I",NULL,socket_control) < 0)
    {
        plog("%s", "Fun(ftp_put):send 'TYPE I' command error.\n\n");
        close(file_put_sock);
        return -1;
    }
    if( ftp_get_reply(socket_control, NULL) == 0)
    {
        plog("%s", "Fun(ftp_put):recv no message after sending 'TYPE I' command.\n\n");
        close(file_put_sock);
        return -1;
    }

    if(mode == 0)
    {
        while(i<3)
        {
            new_sock=accept(file_put_sock,(struct sockaddr *)&local_host,(socklen_t*)&set);
            if(new_sock==-1)
            {
                plog("%s","Fun(ftp_put):error create new_sock in put port.\n\n");
                i++;
                continue ;
            }
            else
                break;
        }
        if(new_sock==-1)
        {
            plog("%s","Fun(ftp_put):The PORT mode won't work.\n\n");
            close(file_put_sock);
            return UPLOAD_PORT_MODE_ERROR;
        }
        while(1)
        {
            count=recv(local_file,send_buff,sizeof(send_buff), MSG_WAITALL);
            if(count<=0)
            {
                fclose(local_file);
                close(file_put_sock);
                close(new_sock);
                plog("%s%s", src_file, "Fun(ftp_put):upload over.\n\n");
                ftp_delay();
                return UPLOAD_SUCCESS;
            }
            else
            {
                write(new_sock,send_buff,sizeof(send_buff));
            }
        }

    }
    else if( mode == 1 )
    {
        while( 1 )
        {
            count = recv( local_file, send_buff, sizeof( send_buff ), MSG_WAITALL);
            if( count <= 0 )
            {
                fclose( local_file );
                close( file_put_sock );
                ftp_delay( );
                plog("%s%s", src_file, "Fun(ftp_put):upload over.\n\n");
                return UPLOAD_SUCCESS;
            }
            else
            {
                write( file_put_sock, send_buff, count );
            }
        }
    }
}

/**************************************************************************************************
 *      function    :   rename upload file in ftp server
 *      para        :   {char * oldName}   	old file name;
 *                      {char * newName} 	new file name;
 *                      {int socket_control} socket file description
 *
 *      return      :   {int} 	-1: error;
								0: OK

 *      history     :   {2013.7.18 wujun} fristly be created
                        {2013.7.29 wujun} modify return data type from void to int
**************************************************************************************************/
int ftp_rename(char *oldName, char *newName, int socket_control)
{
    int error;
    //ftp_get_reply(socket_control);

    if( ftp_send_cmd("RNFR ", oldName, socket_control) < 0 )
    {
        plog("%s%s", oldName, " :send 'RNFR' command error.\n");
        return -1;
    }
    if(  (error = ftp_get_reply(socket_control, NULL)) <= 0 )
    {
        plog("%s","Fun(ftp_rename):recv no message 1.\n");
        return -1;
    }


    if( error == 350 )//350 Ready for RNTO.
    {
        if( (ftp_send_cmd("RNTO ", newName, socket_control)) <= 0 )
        {
            plog("%s", "Fun(ftp_rename): send command 'RNTO' error.\n");
            return -1;
        }
        if( ( error = ftp_get_reply(socket_control, NULL) ) <= 0 )
        {
            plog("%s","Fun(ftp_rename):recv no message 2.\n");
            return -1;
        }

        if(error == 250)//250 Rename successful.
        {
            plog("%s%s%s", "Fun(ftp_rename): rename ", oldName, " successfullu.\n");
            return 0;
        }
        else
        {
            plog("%s","RNTO command excuted failed.\n");
            return -1;
        }

    }else
	{
		plog("%s","RNFR command excuted failed.\n");
		return -1;
	}


}



/*****************************************************************************
 *      function    :   make dir in ftp server
 *      para        :   {char * dirName} dir name;
 *                      {int socket_control} socket file description
 *
 *      return      :   {int} 	-1: error;
								0: OK

 *      history     :   {2013.7.18 wujun} fristly be created
                        {2013.7.29 wujun} modify return data type from void to int
*******************************************************************************/
int ftp_mkdir(char *dirName, int socket_control)
{
    if( dirName == NULL )
    {
        plog("%s", "Fun(ftp_mkdir): dirName is null.\n" );
        return -1;
    }

    int len =  strlen(dirName);
    char *path;
    if( len == 0)
    {
        plog("%s", "Fun(ftp_mkdir): dirName content is blank.\n" );
        return -1;
    }

    path = (char *)malloc(len+1);
    if( path == NULL )
    {
        plog("%s", "Fun(ftp_mkdir): malloc 'path' error.\n" );
        return -1;
    }
    memset(path,0,len+1);
    strcpy(path, dirName);

    int p;

    for( p = 1; p <= len; p++ )
    {
        if( path[p] == '/')
        {
            path[p] = 0;

            if( ( ftp_send_cmd("MKD ", path, socket_control) ) <= 0 )
            {
                plog("%s", "Fun(ftp_mkdir) : send command MKD error.\n");
            }
            if(  ftp_get_reply(socket_control, NULL) <= 0 )
            {
                plog("%s","Fun(ftp_mkdir) :recv no message 3 or network error.\n");
                return -1;
            }
            path[p] = '/';
        }

    }

    plog("%s%s", path, ": create successfully\n.");
    return 0;

}


