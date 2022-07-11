#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <fstream>
#include <memory>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<sys/stat.h>
#include<string.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<stdarg.h>
#include<errno.h>
#include<sys/uio.h>
#include"locker.h"

class http_conn{
public:
    //文件名的最大长度
    static const int FILENAME_LEN = 200;
    //读缓冲区的大小
    static const int READ_BUFFER_SIZE = 2048;
    //写缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;
    //HTTP请求方法，这里仅支持get
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    //解析客户请求时，主状态机所处的状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    //服务器处理HTTP请求的可能结果
    enum HTTP_CODE{
        NO_REQUEST,GET_REQUEST, BAD_REQUEST,
        NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, DYNAMIC_REQUEST,
        INTERNAL_ERROR, CLOSED_CONNECTION
    };
    //行的读取状态
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
public:
    http_conn(){}
    ~http_conn(){}

    //初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr);
    //关闭连接
    void close_conn(bool real_close = true);
    //处理客户请求
    void process();
    //非阻塞读操作(主线程中执行)
    bool read();
    //非阻塞写操作（主线程中执行)
    bool write();

private:
    //初始化连接
    void init();
    //解析HTTP请求
    HTTP_CODE process_read();
    //填充HTTP应答
    bool process_write(HTTP_CODE ret);

    //下面这一组函数被process_read调用以分析HTTP请求
    //分析请求行（第一行）
    HTTP_CODE parse_request_line(char *text);
    //分析头部字段
    HTTP_CODE parse_headers(char *text);
    //分析请求体（实际上只判断了它是否被完整读入了)
    HTTP_CODE parse_content(char *text);
    //分析请求的入口函数
    HTTP_CODE do_request();
    //创建文件映射
    HTTP_CODE do_file_mmap(std::string url);
    //返回当前行的起始位置
    char* get_line() { return _read_buf + _start_line; }
    
    //获取一行请求
    LINE_STATUS parse_line();


    //下面一组函数被process_write调用以填充HTTP应答
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    //所有socket上的事件被注册到同一个epoll内核事件表上
    static int _epollfd;

    //统计用户数量
    static int _user_count;

private:
    //该http连接的socket和对方的socket地址
    int _sockfd;
    sockaddr_in _address;

    //读缓冲区
    char _read_buf[READ_BUFFER_SIZE];
    //标识读缓冲中 已经读入 的客户数据的最后一个字节的下一个位置
    int _read_idx;
    //当前正在分析的字符在读缓冲区中的位置
    int _checked_idx;
    //当前正在解析的行的起始位置
    int _start_line;

    //写缓冲区
    char _write_buf[WRITE_BUFFER_SIZE];
    //写缓冲区中待发送的字节数
    int _write_idx;
    int _byte_hava_send;
    int _byte_to_send;
    //主状态机当前所处的状态
    CHECK_STATE _check_state;
    //请求方法
    METHOD _method;

    //客户请求的目标文件的完整路径，其内容等于doc_root + _url， doc_root是网站根目录
    char _real_file[FILENAME_LEN];
    //客户请求的目标文件的文件名
    char* _url;
    //HTTP协议版本号，当前仅支持HTTP1.1
    char *_version;
    //主机名
    char *_host;
    //http请求的消息体的长度
    int _content_length;
     int _content_have_read;
    //HTTP请求是否要求保持连接
    bool _linger;

    //客户请求的目标文件被mmap到内存中的起始位置
    char* _file_address;
    //目标文件的状态。通过他可以判断文件是否存在，是否为目录等文件信息
    struct stat _file_stat;
    //采用writev来执行写操作，将多个内存块拼接并一起发送出去。iv_count代表内存块的数量
    struct iovec _iv[2];
    int _iv_count;

    // content体存放的指针
    std::shared_ptr<char[]> _content_buffer;
    std::string _boundary;

    //应答体存放的指针，通常用于保存动态页面的字符串
    std::unique_ptr<std::string> _response_content;
    //测试函数
    void print_to_file(const char* data, const char* filename, int size) {
        std::ofstream binaryio(filename, std::ios::binary);
        binaryio.write(data, size);
        binaryio.close();
    }
};

#endif