#include "PostSolver.h"
#include "DataBase.h"
#include "FormItem.h"
#include "FormDataParser.h"
#include <iostream>
#include <string>
#include <type_traits>
#include "Log.h"

PostSolver::PostSolver(int connfd,
                       bool useCGI,
                       std::shared_ptr<char[]> content,
                       int contentLength,
                       std::string url,
                       std::string boundary,
                       MYSQL* sqlconn,
                       std::string entype)
    : _connfd(connfd),
      _useCGI(useCGI),
      _content(content),
      _contentLength(contentLength),
      _url(url),
      _boundary(boundary),
      _sqlconn(sqlconn),
      _entype(entype),
      _requestLength(0) {
    if (_entype == "multipart/form-data") {
        _parser = std::make_unique<FormDataParser>(
            _content, 0, _contentLength, boundary);
    }
    //如果表单的编码格式不是multipart/form-data，咱也处理不明白
    else {
        _parser.reset(nullptr);
    }
}
//处理主流程
void PostSolver::process() {
    if (_useCGI) {
        processCGI();
    }
    else {
        processLocal();
    }
}
//返回应答信息
std::unique_ptr<std::string> PostSolver::getRequest() {
    return nullptr;
}

// CGI方式处理
void PostSolver::processCGI() {}
//本线程里处理
void PostSolver::processLocal() {
    if (_parser.get() == nullptr)
        return;
    if (_url == "/login.cgi") {
        Login();
    }
    else {
        
    }
}
//各种生成html的文件
void PostSolver::Login() {
    _isFile = true;

    auto items = _parser->parse();
    if (items->empty()) {
        LOG_ERROR("PostSolver::Login:69", "%s\n", "用户请求解析失败!");
        _filePath = "/Error.html";
        return;
    }
    FormItem useritem = items->at(0);
    FormItem passwdItem = items->at(1);
    std::string userName = std::string(useritem.getContent().get(), useritem.getLength());
    std::string passwd = std::string(passwdItem.getContent().get(), passwdItem.getLength());
    std::cout << "get userName:" << userName << std::endl;
    std::cout <<"get passwd:" << passwd << std::endl;

    LOG_INFO("PostSolver::Login", "用户[%s] 尝试登录.\n", userName.c_str());
    try {
        DataBase db(_sqlconn);
        QueryResult result =
            db.query("select * from User where user_name = '" + userName + "'");
        auto rows = result.getRows();
        if (rows.empty()) { //无此用户
            _filePath = "/logError.html";
            return;
        }
        if (rows[0]["password"] == passwd) {
            _filePath = "/welcome.html";
        }
        else {
            _filePath = "/logError.html";
        }

    } catch (MySQLException& e) {
        std::cout << e.what();
        _filePath = "/Error.html";
    }
    

}