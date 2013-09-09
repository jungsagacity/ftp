#include    <sys/types.h>

typedef struct
{
	int frequency;
	union
	{
		int secondPoint[60];//store all second points in every minute, the max num is 60;
		int minPoint[60];//store all second points in every minute, the max num is 60;
	}timePoint;

	char type;
}SampleTimePoint;


int isFileExist(char *filePath);
int getSampleTimePoint( int n );
FILE * writeToFile( int year, int day, int hour, int minute, int second, char * stationName );
