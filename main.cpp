#include "SqlConnectionPool.h"
#include "http_conn.h"
#include "locker.h"
#include "threadpool.h"
#include "SignalHandler.hpp"
#include <Log.h>
#include "Timer.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <memory>
#include <vector>
#include <unistd.h>

#define MAX_FD           65536
#define MAX_EVENT_NUMBER 10000

extern int addfd(int epollfd, int fd, bool one_shot, bool isET);
extern int removefd(int epollfd, int fd);

// //注册信号事件，当发生某一信号时调用handler
// void addsig(int sig, void(handler)(int), bool restart = true) {
//     struct sigaction sa;
//     memset(&sa, 0, sizeof(sa));
//     sa.sa_handler = handler;
//     if (restart) {
//         sa.sa_flags |= SA_RESTART;
//     }
//     sigfillset(&sa.sa_mask);
//     assert(sigaction(sig, &sa, NULL) != -1);
// }

void show_error(int connfd, const char* info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("usage: %s ip_address port_number \n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    //初始化&获得信号处理类对象
    SignalHandler& sig = SignalHandler::getInstance();
    //对端关闭连接时，服务端发送数据会产生sigpipe信号，导致进程停止，因此要忽略sigpipe信号
    sig.ignSig(SIGPIPE);
    sig.addSig(SIGALRM);
    sig.addSig(SIGINT);
    alarm(TIMESLOT / 1000);
    
    //LOG初始化
    Log::getInstance().init(500, 128);
    Log::getInstance().start();

    //timer初始化
    Timer timer;
    //创建线程池
    std::shared_ptr<threadpool<http_conn>> pool(new threadpool<http_conn>(1));
    
    //预先为每个可能的客户链接分配一个http_conn对象
    std::vector<http_conn> user(MAX_FD);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    //无法理解作为服务端设置linger的理由，但据说设置为立即终止连接会导致webbench失败，这里还是让其默认的优雅关闭连接比较好
    // linger tmp = {1, 1};
    // setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    //如下两行是为了避免TIME_WAIT状态，仅用于调试，实际使用应该去掉
    // PS:实际上似乎也没什么用，只要在绑定监听socket之前调用就可以了
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    //因为接收连接是逐个进行的，为了避免listenfd上堆积大量连接事件，这里采用水平触发的方式注册监听事件，只要监听列表有连接请求，都会触发事件
    addfd(epollfd, listenfd, false, false);
    //添加信号事件监听,采用水平触发
    addfd(epollfd, sig.getReadSocket(), false, false);
    
    http_conn::_epollfd = epollfd;
    bool stop = false;
    bool timeout = false;
    //主循环,负责接受连接请求、从sock上读写数据并将任务添加到任务队列中、控制是否关闭连接
    while (!stop) {
        int number = epoll_wait(
            epollfd,
            events,
            MAX_EVENT_NUMBER,
            -1); //最后一个参数-1代表如果没有事件触发，将永远阻塞下去，除非进程收到信号。
        if (number < 0 && errno != EINTR) {
            // epoll失败的情况
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                sockaddr_in cilent_address;
                socklen_t cilent_addrlength = sizeof(cilent_address);
                int connfd = accept(
                    listenfd, (sockaddr*)&cilent_address, &cilent_addrlength);

                if (connfd < 0) {
                    printf("errno is: %d\n", errno);
                }
                if (http_conn::_user_count >= MAX_FD) {
                    show_error(connfd, "服务繁忙！");
                    continue;
                }
                //初始化客户连接
                user[connfd].init(connfd, cilent_address);
                //注册定时器
                timer.addTimer(user[connfd]._event);
                timer.extendTime(user[connfd]._event, TIMESLOT * 3);
            }
            //因为文件描述符占用的缘故，信号事件应该优先于处理socket事件
            else if((sockfd == sig.getReadSocket()) && (events[i].events & EPOLLIN)){
                int signal = 0;
                //通常情况下，管道内只有一字节的数据。极端情况下，我不好说。这里仅考虑一字节的情况
                ret = recv(sig.getReadSocket(), (char*)&signal, 1, 0);
                if (ret == -1 || ret == 0)
                    continue;
                switch (signal) {
                    //定时时间，检查一次时间堆
                case SIGALRM:
                    printf("tick\n");
                    timeout = true;
                    break;
                    //ctrl c事件，优雅关闭
                case SIGINT:
                    stop = true;
                    break;
                default:
                    printf("other event ：%d\n", signal);
                }
            }
            //异常情况，例如对方关闭连接或是出现了其他错误
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                user[sockfd].close_conn();
            }
            //有输入数据，即有用户请求
            else if (events[i].events & EPOLLIN) {
                //根据读取数据的结果，决定是将任务添加到线程池，还是关闭连接
                if (user[sockfd].read()) {
                    //延长定时器
                    timer.extendTime (user[sockfd]._event, TIMESLOT * 3);
                    pool->append(&user[sockfd]);
                }
                else {
                    //删除心跳检测事件
                    timer.delTimer(user[sockfd]._event);
                    user[sockfd].close_conn();
                }
            }
            //当写应答完成之后，会注册EPOLLOUT事件，此时就可以根据写的结果来决定下一步的操作
            else if (events[i].events & EPOLLOUT) {
                if (!user[sockfd].write()) {
                    //删除心跳检测事件
                    timer.delTimer(user[sockfd]._event);
                    user[sockfd].close_conn();
                }
                //延长定时器
                timer.extendTime (user[sockfd]._event, TIMESLOT* 3);
            }
            
            else {
                
            }
        }
        //因为定时事件并不是紧急事件，可以留到最后再处理
        if (timeout) {
            timer.tick();
            alarm(TIMESLOT / 1000);
            timeout = false;
        }
    }
    close(epollfd);
    close(listenfd);
    Log::getInstance().stop();
    return 0;
}