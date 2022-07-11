#include "http_conn.h"
#include "SqlConnectionPool.h"
#include "Log.h"
#include "PostSolver.h"
#include <cstring>
#include <memory>
#include <string>

//定义HTTP响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form =
    "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form =
    "You do not hava permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form =
    "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form =
    "There was an unusual problem serveing the requested file.\n";

//网站根目录
const char* doc_root = "/home/ubuntu/ToyWeb/var/www/html";

//将文件描述符设置为非阻塞模式
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
//将文件描述符设置为阻塞模式
int setblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option & ~O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//往epoll里添加事件监视，默认读就绪事件、边缘触发和管道挂起事件，可选单次触发选项,并将文件描述符设置为非阻塞模式
void addfd(int epollfd, int fd, bool one_shot, bool isET) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    if (isET) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

    /*每个使用ET模式的文件描述符都应该是非阻塞的，否则当数据读写完成后的最后一次调用(如果是非阻塞会返回-1)，会一直阻塞从而造成饥渴状态*/
    setnonblocking(fd);
}

//将注册的文件描述符注销掉,并关闭文件描述符
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

//修改文件描述符注册的事件，即修改为ev + ET + oneshot + rdhup
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::_user_count = 0;
int http_conn::_epollfd = -1;

//关闭与目标客户的连接
void http_conn::close_conn(bool real_close) {
    if (real_close && (_sockfd != -1)) {
        removefd(_epollfd, _sockfd);
        _content_buffer.reset();
        _sockfd = -1;
        _user_count--;
    }
}

//初始化与客服的连接
void http_conn::init(int sockfd, const sockaddr_in& addr) {
    _sockfd = sockfd;
    _address = addr;

    //设置为oneshot和ET模式,每一次接收完数据后都需要重新注册epoll事件
    addfd(_epollfd, sockfd, true, true);
    _user_count++;

    init();
}

void http_conn::init() {
    _check_state = CHECK_STATE_REQUESTLINE; //初始化为查看请求行（第一行）的状态
    _linger = false;
    _method = GET;
    _url = NULL;
    _version = NULL;
    _content_length = 0;
    _host = NULL;
    _start_line = 0;
    _checked_idx = 0;
    _read_idx = 0;
    _byte_hava_send = 0;
    _write_idx = 0;
    _content_have_read = 0;
    _content_buffer.reset();
    memset(_read_buf, 0, READ_BUFFER_SIZE);
    memset(_write_buf, 0, WRITE_BUFFER_SIZE);
    memset(_real_file, 0, FILENAME_LEN);
}

//从状态机,用于读取一行数据
http_conn::LINE_STATUS http_conn::parse_line() {
    char text;
    //循环分析checked_index到read_index-1字节
    for (; _checked_idx < _read_idx; _checked_idx++) {
        //获取当前要分析的字节
        text = _read_buf[_checked_idx];
        //如果读取到回车符，说明可能读取到一个完整的行
        if (text == '\r') {
            //如果读的回车符是最后一个字符，说明没有读完
            if (_checked_idx + 1 == _read_idx) {
                return LINE_OPEN;
            }
            //下一个字符是换行，说明读到完整行，直接返回
            else if (_read_buf[_checked_idx + 1] == '\n') {
                //将回车和换行符都改为0
                _read_buf[_checked_idx++] = '\0';
                _read_buf[_checked_idx++] = '\0';
                return LINE_OK;
            }
            //否则存在语法问题————回车后面不能跟其他字符
            return LINE_BAD;
        }
        //如果读到了换行符，说明也可能读取到一个完整的行，就得看看前一个字符是不是\r
        else if (text == '\n') {
            if (_checked_idx > 1 && _read_buf[_checked_idx - 1] == '\r') {
                _read_buf[_checked_idx++] = '\0';
                _read_buf[_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
bool http_conn::read() {
    if (_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    while (1) {
        //读取请求体和请求头存放于不同的内存空间中
        if (_check_state == CHECK_STATE_CONTENT) {
            //读取请求体
            printf("读取请求体\n");
            bytes_read = recv(
            _sockfd, _content_buffer.get() + _content_have_read, _content_length - _content_have_read, 0);
        }
        else {
            //读取请求头
            //要求sockfd设置为非阻塞模式
            bytes_read = recv(
            _sockfd, _read_buf + _read_idx, READ_BUFFER_SIZE - _read_idx, 0);
        }
        
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        }
        //对方关闭了连接,或者缓存满了
        else if (bytes_read == 0) {
            return false;
        }

        if (_check_state == CHECK_STATE_CONTENT) {
            _content_have_read += bytes_read;
        }
        else {
            _read_idx += bytes_read;
        }
        
    }
    return true;
}

//分析请求行（第一行）例如 GET /index.html HTTP/1.1
// temp为请求行字符串的首地址
http_conn::HTTP_CODE http_conn::parse_request_line(char* text) {
    //返回第一个空格和\t的指针,没有则返回NULL
    _url = strpbrk(text, " \t");
    //请求行不可能没有空格
    if (!_url) {
        return BAD_REQUEST;
    }
    LOG_INFO("parse_request_line 176", "得到请求行:%s", text);
    //切割字符串
    *_url++ = '\0';
    //此时URL指向URL部分的第一个字节

    char* method = text;
    //比较字符串，如果相同返回0
    if (strcasecmp(method, "GET") == 0) { //仅支持GET方法
        _method = GET;
        printf(" GET 请求\n");
    }
    else if (strcasecmp(method, "POST") == 0) {
        _method = POST;
        printf("POST 请求\n");
    }
    else {
        LOG_WARN("parse_request_line 187", "%s%s", "不支持的方法", method);
        printf(" %s 请求，不支持捏\n", method);
        return BAD_REQUEST;
    }

    _url += strspn(
        _url,
        " \t"); //该函数返回第一个不是空格和\t的字符的下标,即跳过空格和\t字符
    char* version = strpbrk(_url, " \t");
    if (!version) {
        return BAD_REQUEST;
    }
    *version++ = '\0'; //把url部分尾部的空格字节替换为字符串结束符
    version += strspn(version, " \t"); //此时version指向version部分的第一个字节
    if (strcasecmp(version, "HTTP/1.1") != 0) { //仅支持HTTP/1.1
        return BAD_REQUEST;
    }
    if (strncasecmp(_url, "http://", 7) == 0) {
        _url += 7;
        _url = strchr(_url, '/'); //返回第一次出现'/'的指针
    }

    if (!_url || _url[0] != '/') {
        return BAD_REQUEST;
    }
    printf("the request URL is: %s\n", _url);
    if (strcmp(_url, "/") == 0) { //处理URL为空的情况
        strcpy(_url, "/index.html");
    }
    // HTTP请求行分析完毕，状态转移到头部字段的分析

    _check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//分析头部字段
http_conn::HTTP_CODE http_conn::parse_headers(char* text) {
    std::string s(text);
    if (text[0] == 0) { //如果直接遇到空行，说明头部字段解析完毕
        //如果HTTP请求有消息体，就需要读取_content_length字节的消息体，状态机转移状态
        if (_content_length != 0) {
            //给请求体分配内存
            _content_buffer = std::shared_ptr<char[]>(new char[_content_length]);
            _check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }

        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            _linger = true;
        }
        printf("the request Connection is :%s\n", text);
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        _content_length = atol(text);
        printf("the request Content-Length is :%s\n", text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        _host = text;
        printf("the request host is :%s\n", text);
    }
    else if (s.substr(0, 13) == "Content-Type:") {
        std::string pattern = "multipart/form-data";
        int i = s.find(pattern, 13);
        if (i < 0)
            printf("i can not handle this content type:%s\n",
                   s.c_str());
        else {
            i = s.find("boundary", 34);
            if (i < 0) {
                printf("get boundary ERROR:%s\n", s.c_str());
            }
            else {
                _boundary = s.substr(i + 9);
                printf("Get boundary:%s\n", _boundary.c_str());
            }
        }
    }
    else {
        //其他头部字段都不处理
        printf("i can not handle this header: %s\n", text);
    }
    return NO_REQUEST;
}

//解析HTTP请求的请求体，但没有解析，只是判断是否被完整读入了
http_conn::HTTP_CODE http_conn::parse_content(char* text) {
    // printf("请求体:%s\n", text);
    //下面这个函数只有第一次调用的时候是有用的
    //当读缓冲区的数据全部读完之后，下面这个函数只会复制0个字节过去
    //数据读取操作由read()成员函数接管
    printf("content_buffer size:%d \n", _content_length);
    printf("content_have_read:%d \n", _content_have_read);
    printf("_read_idx: %d, _checked_idx: %d\n",_read_idx, _checked_idx );
    memcpy(_content_buffer.get() + _content_have_read,
            _read_buf + _checked_idx,
            _read_idx - _checked_idx);
    _content_have_read += _read_idx - _checked_idx;
    _checked_idx = _read_idx;
    printf("安全落地\n");
    if (_content_have_read >= _content_length) {
        print_to_file(_content_buffer.get(), "content.txt", _content_length);
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

//主状态机
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST; //初始化为 不是一个完整的请求
    char* text = NULL;
    //这里写的很巧妙，尝试解释一下
    //当分析请求头分析到空行时，且状态被修改为CHECK_STATE_CONTENT，接下来就得开始分析请求体，而不需要分析行了
    //如果分析行的时候发现不能分析到完整的行，则跳出循环，表示这不是一个完整的请求，需要继续读取数据
    while (
        ((_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
        ((line_status = parse_line()) == LINE_OK)) {
        //获取当前行的起始位置并更新行的起始位置
        text = get_line();
        _start_line = _checked_idx;
        //printf("得到新的http行：%s\n", text);

        switch (_check_state) {
        case CHECK_STATE_REQUESTLINE: {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        }
        case CHECK_STATE_HEADER: {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            if (ret == GET_REQUEST) {
                return do_request();
            }
            break;
        }
        //读取请求头的时候，需要将read_buffer剩下的字符移动到content_buffer中，继续读取剩下的内容
        case CHECK_STATE_CONTENT: {
            ret = parse_content(text);
            if (ret == GET_REQUEST) {
                return do_request();
            }
            //当请求体不完整的时候，仍然需要继续读取数据,直接返回
            return NO_REQUEST;
        }

        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

// 当得到一个完整、正确的HTTP请求时，我们就分析目标文件的属性（因为HTTP请求通常都是请求获得一个文件）。如果目标文件存在、对所有用户可读，且不是目录
// 则使用mmap将其映射到内存地址_file_address处，并告诉调用者获取文件成功
//
http_conn::HTTP_CODE http_conn::do_request() {
    if (_method == GET) {
        return do_file_mmap(_url);
    }
    else if (_method == POST) {
        MYSQL* sqlconn;
        ConnectionGuard cguard(sqlconn, ConnectionPool::GetInstance());
        PostSolver solver(_sockfd,
                          false,
                          _content_buffer,
                          _content_length,
                          _url,
                          _boundary,
                          sqlconn);
        solver.process();
        if (solver.isFile()) {
            return do_file_mmap(solver.getFilePath());
        }
        else {
            _response_content = solver.getRequest();
            if(_response_content == nullptr) return BAD_REQUEST;
            return DYNAMIC_REQUEST;
        }
    }
}

//该函数的作用是在需要返回静态文件资源时，将静态文件创建出一个内存映射
//创建出的内存映射将用于后续的文件发送
//如果操作成功，将返回FILE_REQUEST
http_conn::HTTP_CODE http_conn::do_file_mmap(std::string url) {
    strcpy(_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(_real_file + len, url.c_str(), FILENAME_LEN - len - 1);
    //获取文件状态，同时判断是否存在
    if (stat(_real_file, &_file_stat) < 0) {
        return NO_RESOURCE;
    }
    //判断是否可读
    if (!(_file_stat.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }
    //判断是否为文件夹
    if (S_ISDIR(_file_stat.st_mode)) {
        return BAD_REQUEST;
    }

    int fd = open(_real_file, O_RDONLY);
    //创建一个内存映射
    _file_address =
        (char*)mmap(NULL, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}


//对内存映射区执行munmap操作
void http_conn::unmap() {
    if (_file_address) {
        //销毁内存映射
        munmap(_file_address, _file_stat.st_size);
        _file_address = NULL;
    }
}

//写HTTP响应,在触发写就绪事件后执行
//这份代码并不能实现大文件的传输，超出了buffer的size只会发送第一次
bool http_conn::write() {
    int temp = 0;
    //如果没有什么要写的(那为什么会触发)，就将任务清空
    if (_byte_to_send == 0) {
        modfd(_epollfd, _sockfd, EPOLLIN);
        init();
        return true;
    }
    while (1) {
        //将包含应答头和应答体的内存块写入
        temp = writev(
            _sockfd,
            _iv,
            _iv_count); //如果发送缓冲区满了，返回-1；如果没满，但发不完，就尽量发送发送缓冲区的东西，返回发送的字节数，提示下次再发。两种情况都会设置EAGAIN
        if (temp <= -1) {
            //如果TCP写缓冲没有空间，则等待下一轮EPOLLOUT事件。虽然在这一期间，服务器无法接收到同一客户的下一请求，但可以保证连接的完整性
            //顺带一提，目前代码版本由于没有记录当前发送的状态，因此即使下一轮epollout事件到达，他也无法发送完剩余的内容
            if (errno == EAGAIN) {
                LOG_INFO("write", "%s", "缓冲区已满");
                modfd(_epollfd, _sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        LOG_INFO("write", "发送了%d字节数据", temp);
        _byte_hava_send += temp;
        _byte_to_send -= temp;
        if (_byte_hava_send >= _iv[0].iov_len) {
            _iv[0].iov_len = 0;
            _iv[1].iov_base = _file_address + (_byte_hava_send - _write_idx);
            _iv[1].iov_len = _byte_to_send;
        }
        else {
            _iv[0].iov_base = _write_buf + _byte_hava_send;
            _iv[0].iov_len = _iv[0].iov_len - _byte_hava_send;
        }

        //如果已经发送的数据大于等于需要发送的数据，说明已经发送完毕
        if (_byte_to_send <= 0) {
            //发送HTTP响应成功，根据HTTP请求中的Connection字段决定是否关闭连接
            unmap();
            if (_linger) {
                init();
                modfd(_epollfd, _sockfd, EPOLLIN);
                return true;
            }
            else {
                modfd(_epollfd, _sockfd, EPOLLIN);
                return false;
            }
        }
    }
}

//往写缓冲区中写入待发送的数据
bool http_conn::add_response(const char* format, ...) {
    //如果缓冲区满了，直接返回
    if (_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    //获取 参数列表
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(_write_buf + _write_idx,
                        WRITE_BUFFER_SIZE - 1 - _write_idx,
                        format,
                        arg_list);
    //如果写满了，直接返回
    if (len >= (WRITE_BUFFER_SIZE - 1 - _write_idx)) {
        return false;
    }
    _write_idx += len;
    va_end(arg_list);
    return true;
}

//添加状态行
bool http_conn::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

//添加应答头(也就三行)
bool http_conn::add_headers(int content_len) {
    add_content_length(content_len);
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_content_length(int content_len) {
    return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger() {
    return add_response("Connection: %s\r\n",
                        (_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() { return add_response("%s", "\r\n"); }

bool http_conn::add_content(const char* content) {
    return add_response("%s", content);
}

//根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret) {
    case INTERNAL_ERROR: {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form)) {
            return false;
        }
        break;
    }
    case BAD_REQUEST: {
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form)) {
            return false;
        }
        break;
    }
    case NO_RESOURCE: {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form)) {
            return false;
        }
        break;
    }
    case FORBIDDEN_REQUEST: {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form)) {
            return false;
        }
        break;
    }
    case FILE_REQUEST: {
        add_status_line(200, ok_200_title);
        if (_file_stat.st_size != 0) {
            add_headers(_file_stat.st_size);
            _iv[0].iov_base = _write_buf;
            _iv[0].iov_len = _write_idx;
            _iv[1].iov_base = _file_address;
            _iv[1].iov_len = _file_stat.st_size;
            _byte_to_send += _write_idx + _file_stat.st_size;
            _iv_count = 2;
            return true;
        }
        else {
            const char* ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string)) {
                return false;
            }
        }
    }
    default:
        return false;
    }
    //不是传文件的话只需要返回一个应答数据就行了
    _iv[0].iov_base = _write_buf;
    _iv[0].iov_len = _write_idx;
    _iv_count = 1;
    _byte_to_send += _iv[0].iov_len;
    return true;
}

//由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void http_conn::process() {
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) { //如果请求不完整，等待下次读取
        modfd(_epollfd, _sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret) { //返回给客户端的内容生成失败的话，直接关闭连接
        close_conn(true);
    }
    modfd(_epollfd, _sockfd, EPOLLOUT);
}