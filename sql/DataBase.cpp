#include "DataBase.h"
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <algorithm>
DataBase::DataBase(MYSQL* conn) : _db(conn), _autoClose(false){};

DataBase::DataBase(const std::string& userName,
         const std::string& password,
         const std::string& database,
         const std::string& host ,
         const unsigned int port ,
         const char* unix_socket ,
         const unsigned long client_flag):_autoClose(true), _db(NULL){
    _db = mysql_init(_db);
    if (!mysql_real_connect(_db,
                            host.data(),
                            userName.data(),
                            password.data(),
                            database.data(),
                            port,
                            unix_socket,
                            client_flag) ||
        mysql_query(_db, "set names utf8") || mysql_set_character_set(_db, "utf8")) {
        
        std::cout << mysql_error(_db) <<  std::endl;
        throw MySQLConnectException(mysql_error(_db));
    }
}

QueryResult DataBase::query(const std::string& str) {
    if (mysql_real_query(_db, str.data(), str.size())) {
        throw MySQLExecuteException(std::string(mysql_error(_db)));
    }
    //将查询结果全部存储到result中
    MYSQL_RES* result = mysql_store_result(_db);
    //只要是查询语句，result就不会是null
    if (result) {
        QueryResult tmp(*result);
        mysql_free_result(result);
        return tmp;
    }
    //当查询结果为空时，判断他是否应该返回数据
    if (mysql_field_count(_db)) {
        //应该返回数据但什么也没有返回
        throw MySQLExecuteException(mysql_error(_db));
    }
    else {
        //该查询语句执行不应该返回数据，不是一个查询语句
        throw MySQLExecuteException("该语句无返回数据");
    }
}
void DataBase::execute(const std::string& str) {
    if (mysql_real_query(_db, str.data(), str.size()))
        throw MySQLExecuteException(mysql_error(_db));
}

std::string DataBase::getDataBaseName() const { return _db->db; }

DataBase ::~DataBase() {
    if (_autoClose) {
        mysql_close(_db);
    }
}

QueryResult::QueryResult(MYSQL_RES& result) : _fields(new QueryFields(result)) {
    MYSQL_ROW row;
    //构造结果数组
    while ((row = mysql_fetch_row(&result))) {
        // QueryRow r(_fields, row);
        // std::cout << r.getDict().size() << std::endl;
        _rows.emplace_back(_fields, row);
    }
}

const std::vector<QueryRow>& QueryResult::getRows() const noexcept { return _rows; }

QueryRow::QueryRow(std::shared_ptr<QueryFields> fields, MYSQL_ROW row) {
    for (size_t i = 0; i < fields->_fields.size(); i++) {
        //不区分大小写，统一变成小写
        std::string s = fields->_fields[i];
        std::transform(s.begin(), s.end(), s.begin(), tolower);
        _dict[s] = row[i];
    }
}

QueryRow::QueryRow(const QueryRow& row)
    : _fields(row._fields),
      _dict(row._dict){};

QueryRow::QueryRow(QueryRow&& row)
    : _fields(row._fields),
      _dict(std::move(row._dict)){};

const std::unordered_map<std::string, std::string>& QueryRow::getDict() const noexcept {
    return _dict;
}

std::string QueryRow::operator[](std::string index) const {
    //全部转化为小写字符
    std::transform(index.begin(), index.end(), index.begin(), tolower);
    if (_dict.find(index) != _dict.end()) {
        return _dict.at(index);
    }
    else {
        throw MySQLFieldNotFoundException(std::string("未找到名为") + index + std::string("的字符串"));
    }
    return std::string();
}

//封装SQL字段
//该类在一次查询中只会有一个实例，以智能指针的方式存在于相关的类中,不必担心会被提前析构
QueryFields::QueryFields(MYSQL_RES& result) {
    MYSQL_FIELD* fs = mysql_fetch_field(&result);
    const unsigned int fieldCount = mysql_num_fields(&result);
    _fields.reserve(fieldCount);
    for (unsigned int i = 0; i < fieldCount; i++) {
        _fields.push_back(fs[i].name);
    }
}

const std::vector<std::string>& QueryFields::getFields() const noexcept
{
	return _fields;
}