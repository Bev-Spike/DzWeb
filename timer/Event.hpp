#pragma once

#include <cstdio>
#include <functional>
#include <memory>


class Event {
  public:
    Event(): _time(0), _func(std::bind([]{printf("default callback\n");})){}
    Event(uint32_t time) : _time(time) {
        setCallBack([] {
           printf("default callback\n"); 
        });
    }
    Event(const Event& e)=default;
    //允许传入一个函数地址与多个参数作为回调函数
    template <typename F>
    void setCallBack(F f) {
        _func =f;
    }
    template <typename F, typename... Args>
    void setCallBack(F f, Args... args) {
        _func = std::bind(f, args...);
    }

    // template <typename F, typename... Args>
    // void setCallBack(F f, Args&&... args) {
    //     _func = bind(f, std::forward<Args...>(args...));
    // }
    //更新时间为当前开机时间
    void updateTime() {
        timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);
        _time = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    }
    //调用回调函数
    void callBack() { _func(); }
    bool operator<(const Event& e) { return this->_time < e._time; }
    bool operator>(const Event& e) { return this->_time > e._time; }
    bool operator==(const Event& e) { return this->_time == e._time; }

  public:
    //事件过期时间,单位是毫秒
    //之所以用毫秒做单位，因为epoll_wait的timeout就是以毫秒为单位的
    uint32_t _time; 
    std::function<void()> _func;
    //在定时器堆里的索引
    int _pos;
};