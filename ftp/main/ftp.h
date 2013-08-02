#ifndef __FTP_H__
#define __FTP_H__

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>


#define DEFAULT_PORT 21
#define FTP_OK        0
#define FTP_CONNECT_FAILED_FLAG 0


#define     DOWNLOAD_CONNNET_FAILED             1
#define     DOWNLOAD_LOCAL_FILENAME_NULL        2
#define     DOWNLOAD_REMOTE_FILENAME_NULL       3
#define     DOWNLOAD_CREAET_LOCALFILE_ERROR     4
#define     DOWNLOAD_CONNECT_SOCKET_ERROR       5
#define     DOWNLOAD_PORT_MODE_ERROR            6
#define     DOWNLOAD_REMOTE_FILE_NOEXIST        7
#define	 	DOWNLOAD_SUCCESS			        8

#define     UPLOAD_CONNNET_FAILED               9
#define     UPLOAD_LOCAL_FILENAME_NULL          10
#define     UPLOAD_LOCAL_OPEN_ERROR             11
#define     UPLOAD_DATA_SOCKET_ERROR            12
#define     UPLOAD_PORT_MODE_ERROR              13
#define     UPLOAD_SUCCESS                      14





char *strReplace(char *dest, char *src, const char *oldstr, const char *newstr, size_t len);
void plog(char * msg);
int fill_host_addr(char *host_ip_addr,struct sockaddr_in *host,int port);
int ftp_send_cmd(const char* s1, const char* s2, int sock_fd);
int ftp_get_reply(int sock_fd);
int ftp_login(int socket_control, char * user, char * password);
int connectFtpServer(char * server_ip, int port, char * user, char * password);
int rand_local_port();
int xconnect(struct sockaddr_in *s_addr,int type);
int get_port();
int xconnect_ftpdata();
int ftp_get(char* src_file, char * dst_file, int socket_control);
int ftp_put(char* src_file, char * dst_file, int socket_control);

#endif
