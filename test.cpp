#include "Log.h"
#include "SqlConnectionPool.h"
#include "locker.h"
#include "FormDataParser.h"
#include "FormItem.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <pthread.h>
#include"DataBase.h"
#include <sstream>
#include <string>
using namespace std;
void logTest() {
    Log::getInstance().init(10, 128);
    Log::getInstance().start();

    for (int i = 0; i < 20; i++) {
        // sleep(1);
        LOG_DEBUG("main", "%d%s", i, "jest a test");
    }

    sleep(10);
    Log::getInstance().stop();
}

void* sqlTest(void* arg) {

    {
        MYSQL* conn = nullptr;
        ConnectionGuard connguard(conn, ConnectionPool::GetInstance());
        if (!conn) {
            cout << "MySql Connect error!" << endl;
        }

        // 执行命令
        int ret = mysql_query(conn,
                              "INSERT INTO User (`password`, user_name, power) "
                              "VALUES('123', 'bevv', 'normal');");
        if (ret) {
            cout << mysql_stat(conn);
            cout << "error!" << endl;
        }
        else {
            cout << "success!" << endl;
        }
        sleep(3);
    }
}
string url = "localhost";
string user = "root";
string passwd = "594137";
string DBName = "dingzhen";
unsigned int port = 3306;
unsigned int maxconn = 8;

void formTest() {
    std::ifstream fin("/home/ubuntu/ToyWeb/content.txt", ios::in|ios::binary);
    if (fin.is_open()) {
        fin.seekg(0, ios::end);
        int size = fin.tellg();
        fin.seekg(0, ios::beg);
        char *c = new char[size];
        shared_ptr<char[]> p(c);
        int i = 0;
        while (fin.read(p.get(), size)) {
            i+= fin.gcount();
        }

       // cout << p.get() << endl;
        string binary = "----WebKitFormBoundaryUnwawkEWA0JMvvcT";
        FormDataParser fp(p, 0, i, binary);
        auto v = fp.parse();
        for (auto fi : *v) {
            cout << "name: " << fi.getKey() << endl;
            cout << "ContentType: " << fi.getContentType() << endl;
            cout << "filename: " << fi.getFileName() << endl;
            cout << "content: " << string(fi.getContent().get(), fi.getLength())
                 << endl;
            fi.saveContentToFile();
        }
    }
    fin.close();
}

void dbTest() {
    try {
        DataBase db(user, passwd, DBName);
        cout << "DBname: " << db.getDataBaseName() << endl;
        auto result = db.query("select * from User");

        if (result.getRows().size() == 0) {
            cout << "无查询结果!" << endl;
        }
        else {
            for (const QueryRow& row : result.getRows()) {
                cout << row[string("user_name" )];
                cout << endl;
            }
        }
        
    } catch (MySQLException& e) {
        cout << e.what();
    }
}

int main() {
    dbTest();

    // const char* host = "localhost"; //主机名
    // const char* user = "root"; //用户名
    // const char* pwd = "594137"; //密码
    // const char* dbName = "dingzhen"; //数据库名称
    // int port = 3306; //端口号

    // // 创建mysql对象
    // MYSQL *sql = nullptr;
    // sql = mysql_init(sql);
    // if (!sql) {
    //     cout << "MySql init error!" << endl;
    // }
    
    // // 连接mysql
    // sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    // if (!sql) {
    //     cout << "MySql Connect error!" << endl;
    // }

    // // 执行命令
    // std::string s= "select * from user";
    // int ret = mysql_real_query(sql, s.data(), s.size());
    // if (ret) {
    //     cout << mysql_error(sql) << endl;
    //     cout << "error!" << endl;
    // } else {
    //     cout << "success!" << endl;
    // }
    
    // // 关闭mysql
    // mysql_close(sql);
    return 0;
}