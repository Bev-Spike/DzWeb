#include "Log.h"
#include "SqlConnectionPool.h"
#include "locker.h"

#include <iostream>
#include <pthread.h>
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

void* sqlTest(void *arg) {
   
    {
        MYSQL* conn = nullptr;
        ConnectionGuard connguard(conn, ConnectionPool::GetInstance());
        if (!conn) {
            cout << "MySql Connect error!" << endl;
        }

        // 执行命令
        int ret =
            mysql_query(conn, "INSERT INTO User (`password`, user_name, power) VALUES('123', 'bevv', 'normal');");
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
int main() {
    string url = "localhost";
    string user = "root";
    string passwd = "594137";
    string DBName = "dingzhen";
    unsigned int port = 3306;
    unsigned int maxconn = 8;
    ConnectionPool::GetInstance().init(
        url, user, passwd, DBName, port, maxconn);
    pthread_t p1, p2;

    pthread_create(&p1, NULL, sqlTest, NULL);
    pthread_create(&p2, NULL, sqlTest, NULL);
    sleep(10);
    return 0;
}