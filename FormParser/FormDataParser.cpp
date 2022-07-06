#include "FormDataParser.h"
#include "FormItem.h"
#include <memory>
#include <unistd.h>

FormDataParser::FormDataParser(const std::shared_ptr<char[]> data,
                               const int pos,
                               const int length,
                               const std::string boundary)
    : _data(data),
      _pos(pos),
      _dataLength(length),
      _boundary("--" + boundary),
      _lastBoundaryFound(false){}

std::unique_ptr<std::vector<FormItem>> FormDataParser::parse() {
    auto p = std::make_unique<std::vector<FormItem>>();
    int lineStart, lineLength;
    //跳过空白行
    while (getNextLine(lineStart, lineLength)) {
        if (atBoundaryLine(lineStart, lineLength)) {
            break;
        }
    }
    do {
        ItemParam param;
        //处理头部
        parseHeader(param);
        if (atEndOfData())
            break;
        //处理表单数据
        paresFormData(param);
        FormItem formItem(param.partKey,
                          param.partFileName,
                          param.partContentType,
                          _data,
                          param.partDataStart,
                          param.partDataLength);
        //使用移动拷贝以提高效率
        p->push_back(std::move(formItem));
    } while (!atEndOfData());

    return  p;
    
}

//解析表单头部
void FormDataParser::parseHeader(ItemParam& param) {
    int lineStart, lineLength;
    while (getNextLine(lineStart, lineLength)) {
        //头部内容结束后会有一个空白行
        if (lineLength == 0)
            break;

        std::string thisLine = std::string(_data.get() + lineStart, lineLength);
        //获得本请求头的前缀(Content-Type或者Content-Disposition)
        int index = thisLine.find(':');
        if (index < 0) {
            continue;;
        }
        std::string header = thisLine.substr(0, index);
        if (header == "Content-Disposition") {
            param.partKey = getDispositionValue(thisLine, index + 1, "name");
            param.partFileName = getDispositionValue(thisLine, index + 1, "filename");
            
        }
        else if (header == "Content-Type") {
            param.partContentType = thisLine.substr(index + 1);
            trim(param.partContentType);
        }
    }
}
//解析表单元素中的具体数据
void FormDataParser::paresFormData(ItemParam& param) {
    param.partDataStart = _pos;
    param.partDataLength = -1;
    int lineStart;
    int lineLength;
    while (getNextLine(lineStart, lineLength)) {
        //寻找下一行最后都会遇到分界线
        if (atBoundaryLine(lineStart, lineLength)) {
            //内容数据位于分界线前一行
            int indexOfEnd = lineStart - 1;
            if (_data.get()[indexOfEnd] == '\n') {
                indexOfEnd--;
            }
            if (_data.get()[indexOfEnd] == '\r') {
                indexOfEnd--;
            }
            param.partDataLength = indexOfEnd - param.partDataStart + 1;
            break;
        }
    }
    
}
//获取下一行数据,返回行的起始下标和行长度
bool FormDataParser::getNextLine(int& lineStart, int& lineLength) {
    int i = _pos;
    lineStart = -1;

    while (i < _dataLength) {
        //找到一行的末尾（以\n结尾)
        if (_data.get()[i] == '\n') {
            lineStart = _pos;
            lineLength = i - lineStart;
            _pos = i + 1; //下一行的起始位置
            //如果前一个字符是\r的话，不将其计入行的长度
            if (lineLength > 0 && _data.get()[i - 1] == '\r') {
                lineLength--;
            }
            break;
        }
        //到达表单数据的末尾
        if (++i == _dataLength) {
            lineStart = _pos;
            lineLength = i - _pos;
            _pos = _dataLength;
        }
    }
    return lineStart >= 0;
}
//判断是否为边界分割行
//边界的格式为 "--boundary"
//最后边界的格式为"--boundary--"
//为了方便起见，类里面的boundary字符串是直接带有前面两个'-'的
bool FormDataParser::atBoundaryLine(int lineStart, int lineLength) {
    int boundaryLength = _boundary.length();
    //根据长度判断是否为边界
    if (boundaryLength != lineLength && boundaryLength + 2 != lineLength) {
        return false;
    }
    //逐个比较字符
    for (int i = 0; i < boundaryLength; i++) {
        if (_data.get()[i + lineStart] != _boundary[i]) {
            return false;
        }
    }
    if (lineLength == boundaryLength)
        return true;
    //判断是否为最后边界
    if (_data.get()[boundaryLength + lineStart] != '-' ||
        _data.get()[boundaryLength + lineStart + 1] != '-') {
        return false;
    }
    _lastBoundaryFound = true;
    return true;
}
//判断是否到表单元素的末尾
bool FormDataParser::atEndOfData() {
    return _pos >= _dataLength || _lastBoundaryFound;
}
//根据不同的key获取当前行中所需的值
//例如当前行有 name="123", 输入key为"name", 获取"123"
std::string FormDataParser::getDispositionValue(const std::string source,
                                                int pos,
                                                const std::string key) {
    std::string pattern = " " + key + "=";
    int i = source.find(pattern, pos);
    //如果模式不匹配，可能有其他格式
    if (i < 0) {
        pattern = ";" + key + "=";
        i = source.find(pattern, pos);
    }
    if (i < 0) {
        return std::string();
    }

    i += pattern.length();

    if (source[i] == '\"'){
        ++i;
        int j = source.find('\"', i);
        if (i < 0 || i == j) {
            return std::string();
        }
        return source.substr(i, j-i);
    }
    else {
        int j = source.find(";", i);
        if(j < 0){ j = source.size(); }
        auto value = source.substr(i, j - i);
        //去掉前后的空白字符
        return trim(value);
    }
    
}