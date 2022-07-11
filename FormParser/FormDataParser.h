#pragma once
#include <string>
#include <memory>
#include <vector>
//#include"FormItem.h"
//可以做个基类，以拓展不同的分析器。这里还没有实现，太麻烦了= =

class FormItem;

class FormDataParser {
    
    //当前表单的数据信息
    struct ItemParam {
        std::string partKey;
        std::string partFileName;
        std::string partContentType;
        int partDataStart;
        int partDataLength;
    };
    
  public:
    FormDataParser(const std::shared_ptr<char[]> data,
                   const int pos, const int length,
                   const std::string boundary);
    //解析content体，返回一个指向FormItem容器的指针
    std::unique_ptr<std::vector<FormItem>> parse();

  private:
    std::shared_ptr<char[]> _data; //指向表单数据的指针
    int _dataLength;               //表单数据的总长度                                   
    std::string _boundary;         //不同元素的分隔符串

    bool _lastBoundaryFound; //是否找到最后边界
    int _pos;                //当前解析到的位置


  private:
    //解析表单数据的头部
    void parseHeader(ItemParam& param);
    //解析表单元素中的具体数据
    void paresFormData(ItemParam& param);
    //获取下一行数据,返回行的起始下标和行长度
    bool getNextLine(int& lineStart, int& lineLength);
    //判断是否为边界分割行
    bool atBoundaryLine(int lineStart, int lineLength);
    //判断是否到表单元素的末尾
    bool atEndOfData();
    //根据不同的key获取当前行中所需的值
    //例如当前行有name="123", 输入key为"name", 获取"123"
    std::string getDispositionValue(const std::string source,
                                    int pos,
                                    const std::string key);
    
    //去除字符串前后的空白符
    inline std::string& trim(std::string& s) {
        if (s.empty()) {
            return s;
        }
        s.erase(0, s.find_first_not_of(" "));
        s.erase(s.find_last_not_of(" ") + 1);
        return s;
    }
};