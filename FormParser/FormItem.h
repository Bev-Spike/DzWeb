#pragma once
#include <string>
#include <memory>
#include "FormDataParser.h"
class FormItem {
    //允许FormDataParser调用该类的私有成员
    //因为FormItem对象的数据应由FormDataParser来设置
    friend class FormDataParser;

  public:
    //返回该对象元素的文件名（如果存在的话）
    std::string getFileName() const;
    //返回表单元素的key
    std::string getKey() const;
    //返回元素的类型
    std::string getContentType() const;
    //判断对象元素是否为文件
    bool isFile() const;
    //返回对象元素的具体内容(value)
    //考虑到conetnt可能是二进制数据，使用字符数组存储
    //为了防止外界对content作出修改，这里返回的是数据的拷贝
    std::unique_ptr<char[]> getContent() const;


  private:
    FormItem(const std::string name,
             const std::string fileName,
             const std::string contentType,
             const std::shared_ptr<char[]> content,
             const int start,
             const int length);

  private:
    std::string _fileName;
    std::string _key;
    std::string _contentType;
    //指向表单内容,注意，是所有content部分的起始地址
    const std::shared_ptr<char[]> _content;
    //该表单元素的起始下标
    int _dataStart;
    //该表单元素内容的长度
    int _dataLength;
};