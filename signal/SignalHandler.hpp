//该类专门用来处理Linux系统的信号事件
#pragma once

#include <cassert>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
//单例模式,整个程序只需要一个信号处理器就行
class SignalHandler {
  public:
    //信号到来时的处理函数
    static void sigHandler(int sig) {
        int saveErron = errno;
        int msg = sig;
        SignalHandler& instance = SignalHandler::getInstance();
        send(instance.getWriteSocket(), (char*)&msg, 1, 0);
        errno = saveErron;

    }
    static SignalHandler& getInstance() {
        static SignalHandler sighander;
        return sighander;
    }
    //规范化读写端
    int getReadSocket() { return _pipes[0]; }
    int getWriteSocket() { return _pipes[1]; }
    //添加信号处理
    void addSig(int sig, bool restart = true) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SignalHandler::sigHandler;
        if (restart) {
            sa.sa_flags |= SA_RESTART;
        }
        //设定不屏蔽信号（mask已经被置为0）
        sigfillset(&sa.sa_mask);
        //设定sig信号处理方式
        int ret = sigaction(sig, &sa, NULL);
        assert(ret != -1);
    }
    //设置忽略信号
    void ignSig(int sig) { signal(sig, SIG_IGN); }
    //将信号事件恢复为默认
    void cancalSig(int sig) {
        signal(sig, SIG_DFL);
    }
  private:
    //初始化时获取一对套接字
    SignalHandler() {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, _pipes);
        assert(ret != -1);
    }
    ~SignalHandler() = default;
    int _pipes[2];
};