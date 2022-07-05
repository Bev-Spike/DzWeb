#ifndef LOCKER_H
#define LOCKER_H

#include<exception>
#include <memory>
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
    pthread_mutex_t* getMutex();

private:
    pthread_mutex_t _mutex;
};

class cond{
public:
    cond();
    ~cond();
    bool wait(locker &mutex);
    bool signal();
    bool broadcast();
    bool timewait(locker &mutex, struct timespec t);

private:
    pthread_cond_t _cond;
};

//RAII机制的lock
class MutexLockGuard {
  public:
    explicit MutexLockGuard(locker& lock) : locke(lock) {
        locke.lock();
    }

    ~MutexLockGuard() {
        printf("锁守卫析构！\n");
        locke.unlock();
    }
  private:
    locker& locke;
};

#endif