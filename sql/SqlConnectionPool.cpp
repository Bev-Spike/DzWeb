#include "SqlConnectionPool.h"
#include "BlockQueue.h"
#include "Log.h"
#include "locker.h"
#include <cassert>
#include <memory>
#include <mysql/mysql.h>
#include <stdexcept>
using namespace std;

ConnectionPool & ConnectionPool::GetInstance() {
    static ConnectionPool connPool;
	return connPool;
}


ConnectionPool::ConnectionPool()
    : _curConn(0),
      _freeConn(0),
      _maxConn(0),
      _useLog(true) {
        init("82.156.16.46", "root", "594137", "dingzhen", 3306, 2);
      }

ConnectionPool::~ConnectionPool() {
    MutexLockGuard guard(this->_lock);
    //关闭所有连接，清空列表
    //有一个问题就是，当连接池的连接已经发出去了，关闭连接时管不到那些正在使用的连接
    //解决方法只能强制结束用户请求，使其归还连接再释放
    while (!_connQueue->empty()) {
        MYSQL* tmp = nullptr;
        _connQueue->pop(tmp);
        mysql_close(tmp);
    }
    _curConn = 0;
    _freeConn = 0;
    _connQueue->clear();
}


void ConnectionPool::init(string url,
                          string User,
                          string PassWord,
                          string DBName,
                          unsigned int Port,
                          unsigned int MaxConn) {
    this->_url = url;
    this->_user = User;
    this->_password = PassWord;
    this->_databaseName = DBName;
    this->_port = Port;
    this->_maxConn = MaxConn;

    MutexLockGuard guard(_lock);
    _connQueue = make_shared<BlockQueue<MYSQL*>>(_maxConn*2);
    for (int i = 0; i < _maxConn; i++) {
        MYSQL* con = nullptr;
        con = mysql_init(con);
        if (con == nullptr) {
            LOG_ERROR("Connection::init", "%s", "数据库初始化失败！");
            assert(1);
            exit(1);
        }
        con = mysql_real_connect(con,
                                 _url.c_str(),
                                 _user.c_str(),
                                 _password.c_str(),
                                 _databaseName.c_str(),
                                 _port,
                                 nullptr,
                                 0);
        //设置编码
        mysql_set_character_set(con, "utf8");
        if (con == nullptr) {
            LOG_ERROR("Connection::init", "%s", "数据库连接失败！");
            printf("数据库连接失败！\n");
            assert(1);
            exit(1);
        }
        _connQueue->push(con);
        _freeConn++;
    }
}

//当有请求时返回一个可用连接，更新使用和空闲连接数
MYSQL* ConnectionPool::getConnection() {
    MYSQL* con = nullptr;
    //获取一个连接，如果没有连接则阻塞
    _connQueue->pop(con);

    MutexLockGuard guard(_lock);
    _freeConn--;//这两个成员变量好像没有什么用
    _curConn++;
    return con;
}

//释放当前使用的连接
bool ConnectionPool::releaseConnection(MYSQL* conn) {
    if (conn == nullptr) {
        return false;
    }
    _connQueue->push(conn);
    MutexLockGuard guard(_lock);
    _freeConn++;//这两个成员变量好像没有什么用
    _curConn--;

    return true;
}

int ConnectionPool::freeConnCount() { return _freeConn; }

//RAII管理数据库连接
ConnectionGuard::ConnectionGuard(MYSQL*& SQL, ConnectionPool& connPool)
    : _poolRAII(connPool),
      _conRAII(SQL)
    {
    _conRAII = connPool.getConnection();
}

ConnectionGuard::~ConnectionGuard() {
    _poolRAII.releaseConnection(_conRAII);
}