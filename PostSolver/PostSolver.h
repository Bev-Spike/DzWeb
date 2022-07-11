#pragma once
#include <memory>
#include <mysql/mysql.h>
#include <string>
#include "FormDataParser.h"
//可以处理multipart/form-data类型的表单提交post请求
class PostSolver {
  public:
    PostSolver(int connfd,
               bool useCGI,
               std::shared_ptr<char[]> content,
               int contentLength,
               std::string url,
               std::string boundary,
               MYSQL* sqlconn = nullptr,
               std::string entype = "multipart/form-data");
    //处理主流程
    void process();
    //返回应答体
    std::unique_ptr<std::string> getRequest();

    //判断是否获取静态页面
    inline bool isFile() const { return _isFile; }
    inline std::string getFilePath() const { return _filePath; }
    
  private:
    //CGI方式处理
    void processCGI();
    //本线程里处理
    void processLocal();
    //各种生成html的文件
    void Login();
    //......
  private:
    bool _useCGI;
    int _connfd;
    std::shared_ptr<char[]> _content;
    std::string _url;
    int _contentLength;
    std::string _entype;
    std::string _boundary;
    MYSQL* _sqlconn;
    //储存要发送的应答体
    std::unique_ptr<std::string> _request;
    int _requestLength;
    //判断是否发送静态页面
    bool _isFile;
    std::string _filePath;
    //请求体表单处理类
    std::unique_ptr<FormDataParser> _parser;
};

