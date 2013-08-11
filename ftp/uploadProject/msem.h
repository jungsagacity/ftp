#ifndef _MSEM_H_
#define _MSEM_H_
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


int initSem();
int del_sem(int sem_id);
int sem_p(int sem_id);
int sem_v(int sem_id);


#endif
