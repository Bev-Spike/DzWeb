//使用循环数组实现的阻塞队列
//实现了线程安全

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<pthread.h>
#include"locker.h"

template<class T>
class BlockQueue{
public:
    BlockQueue(int maxSize = 1000){
        if(maxSize<=0){
            exit(-1);
        }
        _maxSize = maxSize;
        _array = new T[maxSize];
        _size = 0;
        _front = -1;
        _back = 0;
        _exit = false;
    }

    void clear(){
        _mutex.lock();
        _size = 0;
        _front = -1;
        _back = 0;
        _mutex.unlock();
    }
    //如何通知其他线程这个对象已经析构，这是个安全隐患..
    ~BlockQueue(){
        _mutex.lock();
        if(_array != NULL){
            delete[] _array;
        }
        _mutex.unlock();
    }
    //判断队列是否已满
    bool full(){
        _mutex.lock();
        if(_size>= _maxSize){
            _mutex.unlock();
            return true;
        }
        _mutex.unlock();
        return false;
    }

    //判断是否为空
    bool empty(){
        _mutex.lock();
        if(0 == _size){
            _mutex.unlock();
            return true;
        }
        _mutex.unlock();
        return false;
    }
    //返回队首元素
    bool front(T& value){
        _mutex.lock();
        if(0 == _size){
            _mutex.unlock();
            return false;
        }
        value = _array[(_front + 1) % _maxSize];
        _mutex.unlock();
        return true;
    }
    //返回队尾元素
    bool back(T &value){
        _mutex.lock();
        if(0 == _size){
            _mutex.unlock();
            return false;
        }
        value = _array[(_back + _maxSize - 1) % _maxSize];
        _mutex.unlock();
        return true;
    }


    int size(){
        int tmp = 0;
        _mutex.lock();
        tmp = _size;
        _mutex.unlock();
        return tmp;
    }
    int maxSize(){
        int temp = 0;
        _mutex.lock();
        temp = _maxSize;
        _mutex.unlock();
        return temp;
    }
    //往队尾添加元素，添加元素后将所有阻塞的线程全部唤醒，使其竞争获得元素
    //如果队列已满，也唤醒所有线程
    //若无线程等待条件变量，唤醒则无意义
    bool push(const T& item){
        _mutex.lock();
        if(_size >= _maxSize){
            _cond.broadcast();
            _mutex.unlock();
            return false;
        }
        _array[_back] = item;
        _back = (_back + 1) % _maxSize;


        _size++;
        _cond.broadcast();
        _mutex.unlock();

        return true;
    }

    //取出队首元素，如果队列为空，等待条件变量
    bool pop(T &item){
        _mutex.lock();
        //如果队列为空，则阻塞线程，等待条件变量
        while(_size <= 0){
            //调用wait时，会将当前线程放入阻塞队列中，之后将互斥锁解锁。在返回前，会将互斥锁重新上锁
            if(!_cond.wait(_mutex)){
                //如果条件变量出错，直接返回
                _mutex.unlock();
                return false;
            }
            //取消阻塞状态
            if(_exit){
                _mutex.unlock();
                return false;
            }
        }
        _front = (_front + 1) % _maxSize;
        item = _array[_front];
        _size--;
        _mutex.unlock();
        return true;
    }
    //取出队首元素，如果一定时间后未能取出，直接返回false
    bool pop(T& item, int ms_timeOut){
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        _mutex.lock();
        if(_size <= 0){
            t.tv_sec = now.tv_sec + ms_timeOut / 1000;
            t.tv_nsec = (ms_timeOut % 1000) * 1000;
            if(!_cond.timewait(_mutex, t)){
                _mutex.unlock();
                return false;
            }
        }
        //如果其他线程已经抢先处理了数据，则返回失败
        if(_size <= 0){
            _mutex.unlock();
            return false;
        }
        _front = (_front + 1) % _maxSize;
        item = _array[_front];
        _size--;
        _mutex.unlock();
        return true;
    }
    //强制使阻塞的线程退出
    void quit(){
        _exit = true;
        _cond.broadcast();
    }

private:
    locker _mutex;
    cond _cond;
    T *_array;//队列
    int _size;
    int _maxSize;
    int _front;//总是指向队列的头
    int _back;//总是指向队列尾的下一个元素
    bool _exit;
};

#endif