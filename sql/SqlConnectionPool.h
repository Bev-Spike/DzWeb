#pragma once

#include <stdio.h>
#include <mysql/mysql.h>
#include <string>
#include <iostream>
#include "Log.h"
#include "locker.h"
#include "BlockQueue.h"



class ConnectionPool {
  public:
    MYSQL* getConnection();                     //获取一个数据库连接
    bool releaseConnection(MYSQL* conn);        //释放一个数据库连接                                           
    int freeConnCount();                        //获取空闲连接的数量
    void init(std::string url, std::string User, std::string PassWord, std::string DBName, unsigned int Port, unsigned int MaxConn);
    //单例模式
    static ConnectionPool &GetInstance();

  private:
    ConnectionPool();
    ~ConnectionPool(); //销毁连接池

    unsigned int _maxConn; //最大连接数
    unsigned int _curConn; //已使用连接数
    unsigned int _freeConn; //空闲连接数
    locker _lock;
    std::shared_ptr<BlockQueue<MYSQL*>> _connQueue; //连接池

    std::string _url; //主机地址
    unsigned int _port; //数据库端口号
    std::string _user; //登录数据库用户名
    std::string _password; //数据库密码
    std::string _databaseName; //数据库名
    bool _useLog;//日志开关
};


class ConnectionGuard {
  public:
    ConnectionGuard(MYSQL*& SQL, ConnectionPool& connPool);//传入数据库指针的引用,这样可以将其作为值返回
    ~ConnectionGuard();

  private:
    MYSQL*& _conRAII;
    ConnectionPool& _poolRAII;
};