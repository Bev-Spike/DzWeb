#include "DataBase.h"
#include "Event.hpp"
#include "FormDataParser.h"
#include "FormItem.h"
#include "Log.h"
#include "SqlConnectionPool.h"
#include "locker.h"
#include "MinHeap.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "Timer.h"
#include "SignalHandler.hpp"
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

void sqlTest() {

    {
        MYSQL* conn = nullptr;
        ConnectionGuard connguard(conn, ConnectionPool::GetInstance());
        if (!conn) {
            cout << "MySql Connect error!" << endl;
        }

        // 执行命令
         mysql_set_character_set(conn, "utf8");
        int ret = mysql_query(conn,
                              "INSERT INTO `dingzhen`.`DZmessage`(`dz_describe`, `dz_path`, `dz_appraisal`) VALUES ('椰液', '1657631156639170.jpg', '纯纯的榨种')");
                              //"INSERT INTO `dingzhen`.`User`(`password`, `user_name`) VALUES ('123', '你个小寄吧')");
        if (ret) {
            cout << mysql_stat(conn);
            cout << "error!" << endl;
        }
        else {
            cout << "success!" << endl;
        }
    }
}
string url = "82.156.16.46";
string user = "root";
string passwd = "594137";
string DBName = "dingzhen";
unsigned int port = 3306;
unsigned int maxconn = 8;

void formTest() {
    std::ifstream fin("/home/ubuntu/ToyWeb/content.txt", ios::in | ios::binary);
    if (fin.is_open()) {
        fin.seekg(0, ios::end);
        int size = fin.tellg();
        fin.seekg(0, ios::beg);
        char* c = new char[size];
        shared_ptr<char[]> p(c);
        int i = 0;
        while (fin.read(p.get(), size)) {
            i += fin.gcount();
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
        DataBase db(user, passwd, DBName, url);
        cout << "DBname: " << db.getDataBaseName() << endl;
        auto result = db.query("select * from DZmessage");

        if (result.getRows().size() == 0) {
            cout << "无查询结果!" << endl;
        }
        else {
            for (const QueryRow& row : result.getRows()) {
                cout << row[string("dz_describe")];
                cout << endl;
            }
        }

    } catch (MySQLException& e) {
        cout << e.what();
    }
}

void heapTest() {
    Timer t;
    shared_ptr<Event> e1 = shared_ptr<Event>(new Event(10));
    t.addTimer(e1);
    shared_ptr<Event> e2 = shared_ptr<Event>(new Event(50));
    t.addTimer(e2);
    shared_ptr<Event> e3 = shared_ptr<Event>(new Event(5));
    t.addTimer(e3);
    shared_ptr<Event> e4 = shared_ptr<Event>(new Event(4));
    t.addTimer(e4);

    e4->_time = 1;
    t.adjustTimer(e4);
    cout << endl;

    while (!t.isEmpty()) {
        cout << t.getTop()->_time<< " "  << t.getTop()->_pos << endl;
        t.popTimer();
    }
    cout << endl;
}

void timerTest() {
    shared_ptr<Event> e1 = shared_ptr<Event>(new Event(10));
    e1->updateTime();
    string s = "e1";
    e1->setCallBack(
        [](string s) { printf("我是%s, 牙白，到期了！\n", s.c_str()); }, s);

    shared_ptr<Event> e2 = shared_ptr<Event>(new Event(10));
    sleep(1);
    e2->updateTime();
    s = "e2";
    e2->setCallBack(
        [](string s) { printf("我是%s, 牙白，到期了！\n", s.c_str()); }, s);
    
    Timer t;
    t.addTimer(e1);
    t.addTimer(e2);
    t.extendTime(e1, TIMESLOT);
    t.extendTime(e2, TIMESLOT);
    t.delTimer(e2);
    sleep(7);
    t.tick();
}

void sigTest() {
    alarm(5);
    SignalHandler& sig = SignalHandler::getInstance();
    sig.addSig(SIGALRM);
    sig.addSig(SIGINT);
    sig.addSig(SIGQUIT);
    int signal = 0;
    while (1) {
        recv(sig.getReadSocket(), (char*)&signal, 1, 0);
        cout << signal << endl;
        if (signal == SIGQUIT)
            break;
        if(signal == SIGALRM) alarm(5);
    }
}

int main() {
    sigTest();
    return 0;
}