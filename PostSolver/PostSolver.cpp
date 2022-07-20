#include "PostSolver.h"

#include "DataBase.h"
#include "FormDataParser.h"
#include "FormItem.h"
#include "Log.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

extern const char* doc_root;
const std::string path("./dz");
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
      _requestLength(0),
      _request(std::shared_ptr<std::stringstream>(new std::stringstream)) {
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
    std::unique_ptr<std::string> s =
        std::make_unique<std::string>(_request->str());
    return s;
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
    else if (_url == "/upload.cgi") {
        Upload();
    }
    else if (_url == "/search.cgi") {
        Search();
    }
    else {
    }
}

void PostSolver::addHtmlFront() {
    *_request << "<!DOCTYPE html>\n"
              << "<html>\n";
}
void PostSolver::addHtmlTail() { *_request << "</html>\n"; }
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
    std::string userName =
        std::string(useritem.getContent().get(), useritem.getLength());
    std::string passwd =
        std::string(passwdItem.getContent().get(), passwdItem.getLength());
    std::cout << "get userName:" << userName << std::endl;
    std::cout << "get passwd:" << passwd << std::endl;

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
            _isFile = false;
            addHtmlFront();
            *_request << "<head>\n"
                      << "<meta charset=\"UTF-8\">\n"
                      << "<title>"
                      << "欢迎" << userName << "桑！\n"
                      << "</title>\n"
                      << "</head>\n";

            *_request
                << "<body><br />\n"
                << "<br />\n"
                << "<div align=\"center\">\n"
                << "<font size=\"5\"> <strong>欢迎" << userName << "桑！"
                << "</strong></font>\n"
                << "</div>\n"
                << "<br />\n"
                << "<div  align=\"center\">a w s l</div>\n"
                << " <br />\n"
                << "<br />\n"
                << "<div align=\" center \"><img "
                   "src=\"./resourse/dingzhen.jpg\" title=\"丁真\" /></div>\n";

            addHtmlTail();
        }
        else {
            _filePath = "/logError.html";
        }

    } catch (MySQLException& e) {
        std::cout << e.what();
        LOG_WARN("PostSolver::Login:132", "%s", e.what());
        _isFile = true;
        _filePath = "/Error.html";
    }
}

//处理上传丁真的请求
//根据 上传成功、xx丁真重复两种情况，返回两种动态页面，允许用户再次上传
//  post表单有三项
// text1:xx丁真
// text2:鉴定为xx
//第三项为图片
// 先判断数据库有无重复丁真，之后根据时间给丁真图片命名，保证不会重复，存入数据库成功后再保存图片到本地。

void PostSolver::Upload() {
    _isFile = false;
    auto items = _parser->parse();
    if (items->size() < 3) {
        LOG_ERROR("PostSolver::Login:69", "%s\n", "用户请求解析失败!");
        _isFile = true;
        _filePath = "/Error.html";
        return;
    }
    // xx丁真
    FormItem& text1 = items->at(0);
    //鉴定为xx
    FormItem& text2 = items->at(1);
    //丁真图片
    FormItem& img = items->at(2);
    std::string dzName(text1.getContent().get(), text1.getLength());
    std::string dzAppraisal(text2.getContent().get(), text2.getLength());

    addHtmlFront();
    *_request << "<head>\n"
              << "<meta charset=\"UTF-8\">\n"
              << "<title>上传丁真</title>\n"
              << "</head>\n";
    *_request << "<br>\n"
              << "<br>\n";

    if (dzName.size() > 10 && dzAppraisal.size() > 20) {
        *_request << "<div align=\"center\"><font size=\"5\"> "
                     "<strong>输入内容太长啦！</strong></font></div>\n";
    }
    else {
        //看看这个丁真有没有重复
        try {
            DataBase db(_sqlconn);
            QueryResult result = db.query(
                "select * from DZmessage where dz_describe = '" + dzName + "'");
            auto rows = result.getRows();
            if (!rows.empty()) { //有相同的丁真
                *_request << "<div align=\"center\"><font size=\"5\"> "
                             "<strong>鉴定为重复丁真</strong></font></div>\n";
            }
            else {
                //生成文件名
                std::string oldName = img.getFileName();
                int pos = oldName.find("."); //找到扩展名的位置
                //获取当前时间(以微秒做文件名)
                std::string time = std::to_string(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());
                std::string newFileName = time + oldName.substr(pos);
                std::string filePath = path + "/" + newFileName;
                printf("new filepath:%s \n", filePath.c_str());
                std::string sql =
                    "INSERT INTO `dingzhen`.`DZmessage`(`dz_describe`, "
                    "`dz_path`, `dz_appraisal`) VALUES ('" +
                    dzName + "', '" + filePath + "', '" + dzAppraisal + "');";
                LOG_INFO(
                    "PostSolver::Upload", "执行sql语句：%s\n", sql.c_str());
                db.execute(sql);
                *_request
                    << "<div align=\"center\"><font size=\"5\"> "
                       "<strong>上传成功！再来一张？</strong></font></div>\n";

                //保存图片
                img.saveContentToFile(doc_root + filePath.substr(1));
            }

        } catch (MySQLException& e) {
            std::cout << e.what();
            LOG_WARN("PostSolver::Login:132", "%s", e.what());
            _isFile = true;
            _filePath = "/Error.html";
            return;
        }
    }

    *_request
        << "<br/>\n"
        << "<div class=\"upload-box\">\n"
        << "<form action=\"upload.cgi\" method=\"post\" "
           "enctype=\"multipart/form-data\">\n"
        << "<div align=\"center\"><input type=\"text\" name=\"text1\" "
           "placeholder=\"一眼\" required=\"required\">丁真</div><br/>\n"
        << "<div align=\"center\">鉴定为:<input type=\"text\"  "
           "name=\"text2\" placeholder=\"真\"  "
           "required=\"required\"></div><br/>\n"
        << "<div align=\"center\"><input type=\"file\" multiple=\"multiple\" "
           "accept=\"image/*\" name=\"image\" required=\"required\"></div>\n"
        << "<br>\n"

        << "<div align=\" center \"><button type=\"  "
           "submit\">上传</button></div>\n"
        << "</form>"
        << "</div>\n"
        << "</body>\n";

    addHtmlTail();
}

//检索丁真
void PostSolver::Search() {
    _isFile = false;
    auto items = _parser->parse();
    if (items->empty()) {
        LOG_ERROR("PostSolver::Login:69", "%s\n", "用户请求解析失败!");
        _isFile = true;
        _filePath = "/Error.html";
        return;
    }
    std::string path;
    // xx丁真
    FormItem& text1 = items->at(0);
    std::string dzText =
        std::string(text1.getContent().get(), text1.getLength());
    addHtmlFront();
    *_request << "<head>\n"
              << "<meta charset=\"UTF-8\">\n"
              << "<title>养眼丁真</title>\n"
              << "</head>\n";
    *_request << "<br>\n"
              << "<br>\n";
    if (dzText.size() > 10) {
        *_request << "<div align=\"center\"><font size=\"5\"> "
                     "<strong>没有那么长的丁真!</strong></font></div>\n";
    }
    else {
        //查询是否存在丁真

        try {
            DataBase db(_sqlconn);
            //查询丁真图片的路径
            QueryResult result = db.query(
                "select dz_path from DZmessage where dz_describe = '" +
                dzText + "'");
            auto rows = result.getRows();
            if (rows.empty()) {
                *_request << "<div align=\"center\"><font size=\"5\"> "
                             "<strong>查无丁真！</strong></font></div>\n";
                path = "./dz/null.png";
            }
            else {
                path = rows[0]["dz_path"];
            }

        } catch (MySQLException& e) {
            std::cout << e.what();
            LOG_WARN("PostSolver::Search", "%s", e.what());
            _isFile = true;
            _filePath = "/Error.html";
            return;
        }
    }
    *_request << "<div align=\"center\"><font size=\"5\"> "
                 "<strong>检索</strong></font></div>\n";
    *_request << "<br>\n";
    *_request << "<div class=\"search\">\n";
    *_request << "<form action=\"search.cgi\" method=\"post\" "
                 "enctype=\"multipart/form-data\">\n";
    *_request << "<div align=\"center\"><input type=\"text\" name=\"text1\" "
                 "placeholder=\"一眼\" required=\"required\">丁真</div><br>\n";
    *_request << "<div align=\"center\"><button "
                 "type=\"submit\">确定</button></div>\n";
    *_request << "</form>\n";
    *_request << "</div>\n";
    *_request << "<div align=\"center\"><img src=\"" << path
              << "\" title=\"awsl\"/></div>\n";
    
    *_request << "</body>\n";
    addHtmlTail();
}
