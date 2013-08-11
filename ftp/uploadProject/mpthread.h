#ifndef	_MPTHREAD_H_
#define	_MPTHREAD_H_ 1

#include <pthread.h>

/*
**************************************download thread*******************************************
*	be uesd to lock the gloable variable downloadList[1000] declared in the file "download.h" .
*   whenever log_data is used, we should lock it in case of resource competition.
*/
pthread_mutex_t downloadMutex = PTHREAD_MUTEX_INITIALIZER;	//initialise the mutex lock
//pthread_cond_t downladCond = PTHREAD_COND_INITIALIZER;		//initialise the condition variable


/*
*******************************************upload thread****************************************
*	be uesd to lock the gloable variable uploadList declared in the file "upload.h" .
*   whenever uploadList is used, we should lock it in case of resource competition.
**********************************************************************************************/
pthread_mutex_t uploadMutex = PTHREAD_MUTEX_INITIALIZER;	//initialise the mutex lock



/*
*******************************************log thread****************************************
*	be uesd to lock the gloable variable elog declared in the file "global.h" .
*   whenever elog is used, we should lock it in case of resource competition.
**********************************************************************************************/
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;	//initialise the mutex lock


#endif




