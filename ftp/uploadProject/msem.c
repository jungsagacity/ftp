
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>


#include "msem.h"


int giSemUpload;
int giSemDownload;
int giSemLog;


int initSem()
{
    union semun sem_unionUp,sem_unionDw,sem_unionLog;
    key_t sem_keyUp;
    key_t sem_keyDw;
    key_t sem_keyLog;

    sem_keyUp = ftok(".",1);
    sem_keyDw = ftok(".",2);
    sem_keyLog = ftok(".",3);
    giSemUpload = semget(sem_keyUp,1,0666|IPC_CREAT);
    giSemDownload = semget(sem_keyDw,1,0666|IPC_CREAT);
    giSemLog = semget(sem_keyLog,1,0666|IPC_CREAT);

    sem_unionUp.val = 1;
    if (semctl(giSemUpload,0,SETVAL,sem_unionUp)==-1)
    {
        printf("Sem up init error.\n");
        return -1;
    }

    sem_unionDw.val = 1;
    if (semctl(giSemDownload,0,SETVAL,sem_unionDw)==-1)
    {
        printf("Sem dw init error.\n");
        return -1;
    }

    sem_unionLog.val = 1;
    if (semctl(giSemLog,0,SETVAL,sem_unionLog)==-1)
    {
        printf("Sem log init error.\n");
        return -1;
    }
}


// 删除sem_id信号量
int del_sem(int sem_id)
{
    union semun sem_union;
    if (semctl(sem_id,0,IPC_RMID,sem_union)==-1)
    {
        printf("Sem delete");
        exit(1);
    }
    return 0;
}
// 对sem_id执行p操作
int sem_p(int sem_id)
{
    struct sembuf sem_buf;
    sem_buf.sem_num=0;//信号量编号
    sem_buf.sem_op=-1;//P操作
    sem_buf.sem_flg=SEM_UNDO;//系统退出前未释放信号量，系统自动释放
    if (semop(sem_id,&sem_buf,1)==-1)
    {
        printf("Sem P operation");
        exit(1);
    }
    return 0;
}
// 对sem_id执行V操作
int sem_v(int sem_id)
{
    struct sembuf sem_buf;
    sem_buf.sem_num=0;
    sem_buf.sem_op=1;//V操作
    sem_buf.sem_flg=SEM_UNDO;
    if (semop(sem_id,&sem_buf,1)==-1)
    {
        printf("Sem V operation");
        exit(1);
    }
    return 0;
}




/*
int main() {
    pid_t pid;
    int sem_id;
    key_t sem_key;

    sem_key=ftok(".",'a');
    //以0666且create mode创建一个信号量，返回给sem_id
    sem_id=semget(sem_key,1,0666|IPC_CREAT);
    //将sem_id设为1
    init_sem(sem_id,1);

    if ((pid=fork())<0) {
        perror("Fork error!\n");
        exit(1);
    } else if (pid==0) {
        sem_p(sem_id); //    P操作
        printf("Child running...\n");
        sleep(DELAY_TIME);
        printf("Child %d,returned value:%d.\n",getpid(),pid);
       sem_v(sem_id); //    V操作
        exit(0);
    } else {
        sem_p(sem_id); //    P操作
        printf("Parent running!\n");
        sleep(DELAY_TIME);
        printf("Parent %d,returned value:%d.\n",getpid(),pid);
        sem_v(sem_id); //    V操作
        waitpid(pid,0,0);
        del_sem(sem_id);
        exit(0);
    }

}

*/

