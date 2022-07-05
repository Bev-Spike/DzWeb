#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "locker.h"

#include <cstdio>
#include <exception>
#include <list>
#include <pthread.h>

//线程池，将它定义为模板类是为了代码复用，T是任务类，要求有init和process成员函数
template <typename T>
class threadpool {
  public:
    //参数thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量‘
    threadpool(int thread_number = 8, int max_requests = 10000)
        : _thread_number(thread_number),
          _max_requests(max_requests),
          _stop(false),
          _threads(NULL) {
        if ((thread_number <= 0) || (max_requests <= 0)) {
            throw std::exception();
        }
        _threads = new pthread_t(_thread_number);
        if (!_threads) {
            throw std::exception();
        }

        //创建thread_number个线程,并将他们都设置为脱离线程
        //脱离线程可以使子线程脱离主线程的控制，使得该线程执行完毕后自动释放所有资源
        for (int i = 0; i < thread_number; i++) {
            printf("创建第 %d 个线程\n", i);
            if (pthread_create(_threads + i, NULL, worker, this) != 0) {
                delete[] _threads;
                throw std::exception();
            }
            if (pthread_detach(_threads[i])) {
                delete[] _threads;
                throw std::exception();
            }
        }
    }

    ~threadpool() {
        delete[] _threads;
        _stop = true;
    }
    //往请求队列中添加任务
    bool append(T* request) {
        //操作工作队列时一定要加锁
        _queuelocker.lock();
        if (_workqueue.size() > _max_requests) {
            _queuelocker.unlock();
            return false;
        }
        _workqueue.push_back(request);
        _queuelocker.unlock();
        //信号量 +1,唤醒等待请求的工作线程
        _queuestat.post();
        return true;
    }

  private:
    //工作线程运行的函数，他不断地从工作队列中取出任务并执行之
    static void* worker(void* arg) {
        threadpool* pool = (threadpool*)arg;
        pool->run();
        return pool;
    }
    void run() {
        while (!_stop) {
            //等待信号量>0，争夺请求队列中的请求
            _queuestat.wait();
            _queuelocker.lock();
            if (_workqueue.empty()) { //发现被其他线程抢走了请求
                _queuelocker.unlock();
                continue;
            }
            T* request = _workqueue.front();
            _workqueue.pop_front();
            _queuelocker.unlock();

            if (!request) {
                continue;
            }
            //运行请求中定义好的程序
            request->process();
        }
    }

  private:
    int _thread_number;       //线程池中的线程数
    int _max_requests;        //请求队列中允许的最大请求数
    pthread_t* _threads;      //描述线程池的数组
    std::list<T*> _workqueue; //请求队列
    locker _queuelocker;      //用于保护请求队列的互斥锁
    sem _queuestat;           //是否有任务需要处理
    bool _stop;               //是否结束线程
};

#endif