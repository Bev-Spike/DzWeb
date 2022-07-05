#include<locker.h>
#include<thread>
#include <condition_variable>
using namespace std;

//创建并初始化信号量
sem::sem(){
    if(sem_init(&_sem, 0, 0) != 0){
        throw std::exception();
    }
}
//销毁信号量
sem::~sem(){
    sem_destroy(&_sem);
}

//等待信号量(信号量尝试-1)
bool sem::wait(){
    return sem_wait(&_sem) == 0;
}

//增加信号量(信号量+1)
bool sem::post(){
    return sem_post(&_sem) == 0;
}

//创建并初始化互斥锁
locker::locker(){
    if(pthread_mutex_init(&_mutex, NULL) != 0){
        throw std::exception();
    }
}
//销毁互斥锁
locker::~locker() {
    pthread_mutex_destroy(&_mutex);
}
//上锁
bool locker::lock(){
    return pthread_mutex_lock(&_mutex) == 0;
}
//解锁
bool locker::unlock(){
    return pthread_mutex_unlock(&_mutex) == 0;
}
//返回ptherad_mutex的指针
pthread_mutex_t* locker::getMutex(){
    return &_mutex;
}

//初始化条件变量
cond::cond(){
    if(pthread_cond_init(&_cond, NULL) != 0){
        throw std::exception();
    }
}

//销毁条件变量
cond::~cond(){
    pthread_cond_destroy(&_cond);
}
//等待条件变量
bool cond::wait(locker& mutex){
    int ret = 0;
    ret = pthread_cond_wait(&_cond, mutex.getMutex());
    //pthread_mutex_unlock(&_mutex);
    return ret == 0;
}
//唤醒等待的条件变量
bool cond::signal(){
    return pthread_cond_signal(&_cond) == 0;
}

bool cond::broadcast(){
    return pthread_cond_broadcast(&_cond) == 0;
}

bool cond::timewait(locker& mutex, struct timespec t){
    int ret = 0;
    // pthread_mutex_lock(&m_mutex);
    ret = pthread_cond_timedwait(&_cond, mutex.getMutex(), &t);
    // pthread_mutex_unlock(&m_mutex);
    return ret == 0;
}