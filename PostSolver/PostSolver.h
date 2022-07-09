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
               std::string entype,
               MYSQL* sqlconn = nullptr
               );


  private:
    //各种生成html的文件
    void makeLogin();
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
    //储存要发送的所有数据，包括应答头和HTML内容
    std::unique_ptr<std::string> _request;
    int _requestLength;
    //请求体表单处理类
    std::unique_ptr<FormDataParser> _parser;
};

