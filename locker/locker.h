#ifndef LOCKER_H
#define LOCKER_H

#include<exception>
#include<pthread.h>
#include<semaphore.h>

class sem{
public:
    //创建并初始化信号量
    sem();
    ~sem();
    bool wait();
    bool post();

private:
    sem_t _sem;
};

class locker{
public:
    locker();
    ~locker();

    bool lock();
    bool unlock();

private:
    pthread_mutex_t _mutex;
};

class cond{
public:
    cond();
    ~cond();
    bool wait();
    bool signal();

private:
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
};

#endif