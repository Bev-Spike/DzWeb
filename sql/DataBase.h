//封装数据库操作
//参考https://blog.csdn.net/qq_54121864/article/details/123300314
#pragma once
#include <ios>
#include <iostream>
#include <memory>
#include <mysql/mysql.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

//异常类
class MySQLException : public std::ios_base::failure {
  public:
    MySQLException(const std::string& message) : std::ios_base::failure(message){};
};
//连接异常
class MySQLConnectException : public MySQLException {
  public:
    MySQLConnectException(const std::string& message)
        : MySQLException(message){};
};
//执行异常
class MySQLExecuteException : public MySQLException {
  public:
    MySQLExecuteException(const std::string& message)
        : MySQLException(message){};
};
//数据域异常
class MySQLFieldNotFoundException : public MySQLException {
  public:
    MySQLFieldNotFoundException(const std::string& message)
        : MySQLException(message){};
};

//封装查询操作返回的数据域
class QueryFields {
    friend class QueryRow;
    friend class QueryResult;

  private:
    QueryFields(MYSQL_RES& result);

  public:
    QueryFields(const QueryFields& fileds) = delete; //不允许拷贝构造
    QueryFields(QueryFields&& fields) = delete; //不允许移动拷贝构造
    const std::vector<std::string>& getFields() const noexcept;

  private:
    std::vector<std::string> _fields;
};

//封装查询返回的行
class QueryRow {
  public:
    QueryRow(std::shared_ptr<QueryFields> fields, MYSQL_ROW row);
    QueryRow(const QueryRow& row);
    QueryRow(QueryRow&& row);
    const std::unordered_map<std::string, std::string>& getDict() const noexcept;
    std::string operator[](std::string index) const;
    ~QueryRow() = default;

  private:
    std::unordered_map<std::string, std::string> _dict;
    std::shared_ptr<QueryFields> _fields;
};

class QueryResult {
    friend class QueryRow;

  public:
    QueryResult(MYSQL_RES& result);
    const std::vector<QueryRow>& getRows() const noexcept;
    const QueryFields& getFields() const noexcept;
    inline int getRowsSize() const {
        return _rows.size();
    }
  private:
    std::vector<QueryRow> _rows;
    std::shared_ptr<QueryFields> _fields;
};

class DataBase {
  public:
    DataBase(const std::string& userName,
             const std::string& password,
             const std::string& database,
             const std::string& host = "localhost",
             const unsigned int port = 3306,
             const char* unix_socket = nullptr,
             const unsigned long client_flag = 0);
    DataBase(MYSQL* conn);
    void execute(const std::string& str);
    QueryResult query(const std::string& str);
    std::string getDataBaseName() const;
    ~DataBase();

  private:
    MYSQL* _db;
    bool _autoClose;

};