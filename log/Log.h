#ifndef _LOG_H_
#define _LOG_H_

#include<string>
#include<pthread.h>
#include"BlockQueue.h"
#include<stdarg.h>
#include<cstring>
#include"locker.h"
#include<unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include<stdio.h>

//这四个宏定义在其他文件中使用，主要用于不同类型的日志输出
#define LOG_DEBUG(funcname, format, ...) Log::getInstance().writeLog(0, funcname, format, __VA_ARGS__)
#define LOG_INFO(funcname, format, ...) Log::getInstance().writeLog(1, funcname, format, __VA_ARGS__)
#define LOG_WARN(funcname, format, ...) Log::getInstance().writeLog(2, funcname, format, __VA_ARGS__)
#define LOG_ERROR(funcname, format, ...) Log::getInstance().writeLog(3, funcname, format, __VA_ARGS__)

class Log{
public:
    //C++11后，使用局部变量懒汉不用加锁
    //获取全局唯一一个实例
    static Log& getInstance(){
        static Log instance;
        return instance;
    }
    //初始化
    bool init(int maxLines = 500, int maxQueueSize = 128);
    //开始记录
    bool start();
    bool stop();
    //格式化制作日志，并将日志加入阻塞队列中
    //日志包含当前线程、时间、等级
    bool writeLog(int level, const char* funcName, const char* format, ...);

private:
    //将字符串加入队列
    void addQueue(std::string s);
    //检查文件状态，将字符串写入文件中
    bool writeToFile(std::string& s);
    //创建文件
    FILE* creatFile();
    void flush();
    static void* threadFunc(void* args);
    Log()=default;
    ~Log();


private:
    char _fileName[128]; //当前文件名
    int _fileIdx;
    int _today;
    FILE *_fp;
    int _maxLines;//行数
    int _lineCounts;//当前行数
    BlockQueue<std::string>* _logQueue;
    locker _mutex;
    bool _quit = false;
    pthread_t _pid;
};

#endif