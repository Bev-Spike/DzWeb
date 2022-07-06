#include "FormItem.h"
#include <cstring>
#include <memory>
#include <fstream>

//返回该对象元素的文件名（如果存在的话）
std::string FormItem::getFileName() const {
    return _fileName;
}
//返回表单元素的key
std::string FormItem::getKey() const {
    return _key;
}
//返回元素的类型
std::string FormItem::getContentType() const {
    return _contentType;
}
//判断对象元素是否为文件
bool FormItem::isFile() const {
    return !_fileName.empty();
}
//返回对象元素的具体内容(value)
//考虑到conetnt可能是二进制数据，使用字符数组存储
//为了防止外界对content作出修改，这里返回的是数据的拷贝
std::unique_ptr<char[]> FormItem::getContent() const {
    std::unique_ptr<char[]> p = std::make_unique<char[]>(_dataLength);
    std::strncpy(p.get(), _content.get()+ _dataStart, _dataLength);
    return p;
}
//返回conetn的长度
int FormItem::getLength() const {
    return _dataLength;
}
FormItem::FormItem(const std::string key,
                   const std::string fileName,
                   const std::string contentType,
                   const std::shared_ptr<char[]> content,
                   const int start,
                   const int length)
    : _key(key),
      _fileName(fileName),
      _contentType(contentType),
      _content(content),
      _dataStart(start),
      _dataLength(length) {}

//将content以文件的方式保存
//如果没有指定文件名，则以_filename为文件名保存
//如果没有指定文件名且_filename为空，则返回false
bool FormItem::saveContentToFile(std::string filename) const {
    if (filename.empty() && _fileName.empty()) {
        return false;
    }
    std::string name;
    if(!filename.empty()){
        name = filename;
    }
    else {
        name = _fileName;
    }
    std::ofstream of;
    of.open(name, std::ofstream::binary);
    of.write(_content.get() + _dataStart, _dataLength);
    of.close();

    return true;
}