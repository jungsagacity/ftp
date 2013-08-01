
/*----------------------------------event log --------------------------------------------------
*	we now can record 5 kinds of macro definitons about events, all of them should be written to 
*	specified log file. In the future, maybe we should send some exceptrion,like connecting to ftp
*   sever failed ,upload file interrupt and so on, then the log event type can tell us which event
*   log node'sinformatino should be read and sent to a management software center.
*----------------------------------------------------------------------------------------------*/

#define 	UPLOAD_CONNNET_FAILED		1	
#define 	DOWNLOAD_CONNNET_FAILED		2
#define		UPLOAD_FAILED				3
#define	 	DOWNLOAD_FAILED				4
#define		UPLOAD_SUCCESS				5
#define	 	DOWNLOAD_SUCCESS			6




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
