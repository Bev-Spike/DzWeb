#include<locker.h>

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
locker::~locker(){
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

//初始化条件变量
cond::cond(){
    if(pthread_mutex_init(&_mutex, NULL) != 0){
        throw std::exception();
    }
    if(pthread_cond_init(&_cond, NULL) != 0){
        pthread_mutex_destroy(&_mutex);
        throw std::exception();
    }
}

//销毁条件变量
cond::~cond(){
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}
//等待条件变量
bool cond::wait(){
    int ret = 0;
    //因为对条件变量的修改是在临界区操作的，因此需要对其加锁,防止错过唤醒信号
    pthread_mutex_lock(&_mutex);
    ret = pthread_cond_wait(&_cond, &_mutex);
    pthread_mutex_unlock(&_mutex);
    return ret == 0;
}
//唤醒等待的条件变量
bool cond::signal(){
    return pthread_cond_signal(&_cond) == 0;
}