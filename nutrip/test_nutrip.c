#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <errno.h>

#include "test_nutrip.h"

SampleTimePoint stp;

int isFileExist(char *filePath)
{
    int len =  strlen(filePath);
    char *path;
    if( len == 0)
    {
        return -1;
    }

    path = (char *)malloc(len+1);
    memset(path,0,len+1);
    strcpy(path, filePath);
    int p;
    for( p = 1; p <= len; p++ )
    {
        if( path[p] == '/')
        {
            path[p] = 0;
            if( access(path, 0) != 0)
            {
                if( mkdir(path, 0777) == -1 )
                {
                    #ifdef DEBUG
                    printf("make dir %s error",path);
                    #endif
                    return -1;
                }
            }
            path[p] = '/';
        }
        else if( path[p] == 0 )
        {
            if( access(path, 0) != 0)
            {
                FILE *fp;
                if( ( fp = fopen( path,"w")) == NULL )
                {
                    #ifdef DEBUG
                    printf("make file %s error %d.\n",path,errno);
                    #endif
                    return -1;
                }
                fclose(fp);
            }
        }
    }

    return 0;
}







int getSampleTimePoint( int n )
{
	if( n == 0)
	{
		printf("set sample frequency error.\n");
		return -1;
	}

	//initialise stp struct.
	int i = 0;
	for(i=0;i<60;i++)
	{
		stp.timePoint.secondPoint[i] = -1;
	}

	//  sample frequency lee than 60 s
	if( n < 60 )
	{
		stp.frequency = n;
		stp.type = 1;

		int round = 60/n;printf("sec:\n");
		for( i = 0; i < round; i ++ )
		{
			stp.timePoint.secondPoint[i] = i*n;
			printf("%d\t", stp.timePoint.secondPoint[i]);
		}printf("\n\n");
	}
	//  sample frequency equal to or more than 60 s
	else
	{
		stp.frequency = n;
		stp.type = 2;

		int freq = 0;
		if( n % 60 )
		{
			freq = n/60 + 1;
		}
		else // n = 60, 120, ....
		{
			freq = n/60;
		}

		int round = 60/freq;printf("min:\n");
		for( i = 0; i < round; i ++ )
		{
			stp.timePoint.minPoint[i] = i*freq;
			printf("%d\t", stp.timePoint.minPoint[i]);
		}printf("\n\n");
	}
}


FILE * writeToFile( int year, int day, int hour, int minute, int second, char * stationName )
{

	if( stationName == NULL )
	{
        printf("station name is NULL.\n");
        return NULL;
	}


	int enableFlag = 0, i = 0;

	//check time point is legal.
	if( stp.type == 1 )
	{
		i = 0;
		while( stp.timePoint.secondPoint[i] >= 0 )
		{
			if( second == stp.timePoint.secondPoint[i] )
			{
				enableFlag = 1;
				break;
			}
			i++;
		}
	}
	else
	{
		i = 0;
		while( stp.timePoint.minPoint[i] >= 0 )
		{
			if( minute == stp.timePoint.minPoint[i] )
			{
				enableFlag = 1;
				break;
			}
			i++;
		}
	}


	if( enableFlag == 1)
	{
		char fileName[1024];
		memset(fileName, 0, 1024);

		//RealTime/YYYY/DDD/HOURLY/HOUR/
		sprintf(fileName, "RealTime/%04d/%03d/HOURLY/%02d/%s\n", year, day, hour, stationName);
        printf("filename: %s",fileName);
		// automatic create dir and file if not exsit.
		isFileExist( fileName );

		FILE * fp;
		fp = fopen(fileName, "a+");
		if( fp == NULL )
		{
			printf("Fun(writeToFile): can not open file %s. %s\n", fileName, strerror(errno) );
			return NULL;
		}

		return fp;
	}

	return NULL;
}


/*****************************test*********************************
*                        main
*******************************************************************/

int main()
{
	FILE * fp;
	getSampleTimePoint( 30 );

	int year,day,hour,minute, second;

	year = 2013;
	day = 222;
	for( hour = 0; hour<24; hour ++)
	for( minute = 0; minute <60; minute ++)
	for( second = 0; second <60; second ++)
	{
		//printf("time is : %04d-%03d %02d:%02d:%02d\n", year, day, hour, minute, second);

		fp = writeToFile(year, day, hour, minute, second, "WHU2.txt");

		if( fp )
		{
			printf("time is : %04d-%03d %02d:%02d:%02d\n", year, day, hour, minute, second);

			fclose(fp);
			usleep(1000);
		}
	}

}
