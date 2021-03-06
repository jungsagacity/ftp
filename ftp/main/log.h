
/*----------------------------------event log --------------------------------------------------
*	we now can record 5 kinds of macro definitons about events, all of them should be written to
*	specified log file. In the future, maybe we should send some exceptrion,like connecting to ftp
*   sever failed ,upload file interrupt and so on, then the log event type can tell us which event
*   log node'sinformatino should be read and sent to a management software center.
*----------------------------------------------------------------------------------------------*/

#define     L_DOWNLOAD_CONNNET_FAILED           1
/*
#define     L_DOWNLOAD_LOCAL_FILENAME_NULL      2
#define     L_DOWNLOAD_REMOTE_FILENAME_NULL     3
#define     L_DOWNLOAD_CREAET_LOCALFILE_ERROR   4
#define     L_DOWNLOAD_CONNECT_SOCKET_ERROR     5
#define     L_DOWNLOAD_PORT_MODE_ERROR          6
#define     L_DOWNLOAD_REMOTE_FILE_NOEXIST      7
*/
#define     L_DOWNLOAD_FAILED                   8
#define	 	L_DOWNLOAD_SUCCESS			        9

#define     L_UPLOAD_CONNNET_FAILED             10
/*
#define     L_UPLOAD_LOCAL_FILENAME_NULL        11
#define     L_UPLOAD_LOCAL_OPEN_ERROR           12
#define     L_UPLOAD_DATA_SOCKET_ERROR          13
#define     L_UPLOAD_PORT_MODE_ERROR            14
*/
#define     L_UPLOAD_FAILED                     15
#define     L_UPLOAD_SUCCESS                    16
#define     L_UPLOAD_FILE_NOEXIST               17
#define	 	L_UPLOAD_INTIME	                    18
#define	 	L_UPLOAD_LATE                       19






//record connect,upload,download event unnormal performerce.
typedef struct eventlog
{
    int eventTpye;
	char *fileName;//used when upload or download event occured;
	char startTime[100];
	char endTime[100];
	int isRead;

	struct eventlog * next;

} EventLog;

void initEventLog();
void addEventLog(char eventType, char *con, char * startTime, char *endTime);
int writeEventLog(char *uploadLog, char *downloadLog);
