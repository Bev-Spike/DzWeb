#include "Log.h"
#include <string>

//根目录的名字
const char *rootDir = "/home/ubuntu/ToyWeb/var/logdata/";

//初始化，设置参数，创建文件
bool Log::init(int maxLines, int maxQueueSize ){
    _maxLines = maxLines;
    _fileIdx = 1;
    _fp = creatFile();
    if(!_fp){
        return false;
    }
    _lineCounts = 2;
    _logQueue = new BlockQueue<std::string>(maxQueueSize);
    _quit = false;
    time_t now = time(NULL);
    struct tm *tmstr = localtime(&now);
    _today = tmstr->tm_mday;
    return true;
}
//开始记录
bool Log::start(){
    int ret = pthread_create(&_pid, NULL, threadFunc, (void*)this);
    if(ret!= 0){
        return false;
    }
    return true;
}
bool Log::stop(){
    _mutex.lock();
    flush();
    _quit = true;
    _logQueue->quit();
    _mutex.unlock();
    pthread_join(_pid, nullptr);
    return true;
}
//格式化制作日志，并将日志加入阻塞队列中
//日志包含当前线程、函数名、时间、等级
bool Log::writeLog(int level, const char* funcName, const char *format, ...){
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    char levelStr[16] = {0};
    switch (level){
    case 0:
        strcpy(levelStr, "debug");
        break;
    case 1:
        strcpy(levelStr, "info");
        break;
    case 2:
        strcpy(levelStr, "warn");
        break;
    case 3:
        strcpy(levelStr, "error");
        break;
    default:
        strcpy(levelStr, "info");
        break;
    }

    va_list valist;
    va_start(valist, format);

    std::string log_str;
    char buf[512] = {0};
    //格式长这样：
    //[10:10:59][info][线程id][函数名]格式化信息
    int n = snprintf(buf, 512, "[%02d:%02d:%02d][%s][%lu][%s]",
                     sys_tm->tm_hour,
                     sys_tm->tm_min,
                     sys_tm->tm_sec,
                     levelStr,
                     (unsigned long)pthread_self(),
                     funcName);
    int m = vsnprintf(buf + n, 512 - n, format, valist);
    buf[n + m] = '\n';
    std::string s(buf);
    addQueue(s);
    return true;
}

//将字符串加入队列
void Log::addQueue(std::string s){
    _logQueue->push(s);
}
//检查文件状态，将字符串写入文件中
//只有工作线程会调用这个函数
bool Log::writeToFile(std::string &s){
    time_t now = time(NULL);
    struct tm *tmstr = localtime(&now);

    _mutex.lock();
    //如果文件已满或日期变更，重新创建一个文件
    if(_lineCounts >= _maxLines || tmstr->tm_mday != _today){
        flush();
        _today = tmstr->tm_mday;
        fclose(_fp);
        _fp = creatFile();
        if(!_fp)
            return false;
        _lineCounts = 2;
    }
    _mutex.unlock();

    int icount = fputs(s.c_str(), _fp);
    _lineCounts++;
    return true;
}
//创建文件
FILE* Log::creatFile(){
    time_t now = time(NULL);
    struct tm *tmstr = localtime(&now);
    char newFileName[128] = {0};
    strcpy(newFileName, rootDir);
    char *p = strrchr(newFileName, '/');//从后往前找到第一个/的位置

    char dateStr[32] = {0};
    snprintf(dateStr, 32, "%d_%02d_%d_%d.log", tmstr->tm_year + 1900, tmstr->tm_mon + 1, tmstr->tm_mday, _fileIdx);
    strcpy(p + 1, dateStr);
    struct stat s;
    while (access(newFileName, F_OK) == 0){//如果文件存在，则序号+1，重新生成一个文件名
        _fileIdx++;
        snprintf(dateStr, 32, "%d_%02d_%d_%d.log", tmstr->tm_year + 1900, tmstr->tm_mon + 1, tmstr->tm_mday, _fileIdx);
        strcpy(p + 1, dateStr);
    }

    _today = tmstr->tm_mday;

    FILE *fp = fopen(newFileName, "w+");
    if(!fp){
        printf("error\n");
        return nullptr;
    }
    char initLog[128] = {0};
    snprintf(initLog, 128, "The file was created on %d-%02d-%d  %02d:%02d:%02d. \n\n", 
        tmstr->tm_year + 1900, tmstr->tm_mon + 1, tmstr->tm_mday, tmstr->tm_hour, tmstr->tm_min, tmstr->tm_sec);
    fputs(initLog, fp);

    strcpy(_fileName, newFileName);
    _lineCounts = 3;
    return fp;
}
void Log::flush(){
    //强制刷新写入流缓冲区
    fflush(_fp);
}
void* Log::threadFunc(void* args){
    Log* lp = (Log *)args;
    while (!lp->_quit){
        std::string s;
        lp->_logQueue->pop(s);
        lp->writeToFile(s);
        lp->flush();
    }
    return nullptr;
}

Log::~Log(){
    delete _logQueue;
    fclose(_fp);
}