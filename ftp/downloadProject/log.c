#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.h"

/*-------------------------------------ftp info-------------------------------------------------
*
*----------------------------------------------------------------------------------------------*/
EventLog *elog; //global variable
EventLog *elogTail;//global variable
extern pthread_mutex_t logMutex ;

/**
*    function   :   initialise EventLog header
*    para       :   {void}

*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/
void initEventLog()
{
	elog = (EventLog *)malloc(sizeof(EventLog));
	elog ->next = NULL;

	elogTail = elog;

}



/**
*    function   :   add one event log
*    para       :   {char eventType} event type, like " CONNNET_FTP_SERVER_FAILED, UPLOAD_FAILED,
                                      DOWNLOAD_FAILED, UPLOAD_SUCCESS, DOWNLOAD_SUCCESS"
*    return     :   {void}
*
*    history    :   {2013.7.25 wujun} frist create;
**/
void addEventLog(char eventType, char *fileName, char * startTime, char *endTime)
{
	EventLog *newlog;
	EventLog *oldlogTail = elogTail;

	newlog = (EventLog *)malloc(sizeof(EventLog));
	newlog -> eventTpye = eventType;

	int fileSize = strlen(fileName) + 1;
	newlog -> fileName = (char *)malloc(fileSize);
	memset(newlog -> fileName,0, fileSize);
	strcpy(newlog -> fileName, fileName);

	//set starting time or end time;
	memset(newlog -> startTime, 0, 100);
	strcpy(newlog -> startTime, startTime);
	memset(newlog -> endTime, 0, 100);
	strcpy(newlog -> endTime, endTime);

	newlog -> isRead = 0;

	elogTail = newlog;
	oldlogTail -> next = newlog;
	newlog -> next = NULL;

}


/**
*    function   :   write event to file
*    para       :   {char *uploadLog} upload log file name;
					{char *downloadLog} download log file name;
*    return     :   {int} 	-1:filename is null;
							0:OK;
*
*    history    :   {2013.7.25 wujun} frist create;
**/
int writeEventLog(char *uploadLog, char *downloadLog)
{
	EventLog *newlog = elog->next;

	FILE *upLogFp;
	FILE *dwLogFp;

	if(strlen(uploadLog)== 0 || strlen(downloadLog)== 0)
		return -1;

	if( (upLogFp=fopen( uploadLog ,"at") ) == NULL )
	{
		upLogFp = fopen( uploadLog ,"wt");
	}

	if( (dwLogFp=fopen( downloadLog ,"at") ) == NULL )
	{
		dwLogFp = fopen( downloadLog ,"wt");
	}

	while(newlog != NULL)
	{
		switch(newlog->eventTpye)
		{
			case L_UPLOAD_CONNNET_FAILED:
			{
				fprintf(upLogFp,"%s\tUPLOAD CONNNET FAILED\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_DOWNLOAD_CONNNET_FAILED:
			{
				fprintf(dwLogFp,"%s\tDOWNLOAD CONNNET FAILED\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_UPLOAD_FAILED:
			{
				fprintf(upLogFp,"%s\tUPLOAD FAILED\t\t\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_UPLOAD_INTIME:
			{
				fprintf(upLogFp,"%s\tUPLOAD INTIME\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_UPLOAD_LATE:
			{
				fprintf(upLogFp,"%s\tUPLOAD LATE\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_UPLOAD_FILE_NOEXIST:
			{
				fprintf(upLogFp,"%s\tFILE NOEXIST\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_DOWNLOAD_FAILED:
			{
				fprintf(dwLogFp,"%s\tDOWNLOAD FAILED\t\t\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_UPLOAD_SUCCESS:
			{
				fprintf(upLogFp,"%s\tUPLOAD SUCCESS\t\t\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			case L_DOWNLOAD_SUCCESS:
			{
				fprintf(dwLogFp,"%s\tDOWNLOAD SUCCESS\t\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
			default:
			{
				//fprintf(fp,"%s\tUNKOWN\t\t\t\t\t\t%s\t%s\n",newlog->startTime,newlog->fileName,newlog->endTime);
				break;
			}
		}

		//delete struct list node.
		elog->next = newlog->next;
		free(newlog->fileName);
		free(newlog);

		newlog = elog->next;

	}
	elogTail = elog;

	fclose(dwLogFp);
	fclose(upLogFp);

	return 0;
}

