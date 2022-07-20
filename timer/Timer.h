#pragma once

#include "Event.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#define TIMESLOT 5000  //时间单位（毫秒)
class Timer {
  public:
    //添加定时器
    void addTimer(std::shared_ptr<Event> e);
    //删除目标定时器
    void delTimer(std::shared_ptr<Event> e);
    //获得顶部定时器
    std::shared_ptr<Event> getTop();
    //删除顶部定时器
    void popTimer();
    //修改目标定时器
    //如果目标定时器不存在，将其插入堆中
    void adjustTimer(std::shared_ptr<Event> e);
    //延长某个定时器的时间
    void extendTime(std::shared_ptr<Event> e, std::uint32_t time);
    //获得定时器堆的大小
    inline int getSize() { return _heap.size(); }
    //判断是否为空
    inline bool isEmpty() { return _heap.empty(); }
    //获取当前时间，将过期事件清除
    void tick();
    
  private:
    //将第hole个元素下虑，确保以第hole个结点作为根的子树拥有最小堆性质
    void percolateDown(int hole);
    //将第hole个元素上浮
    void percolateUp(int hole) ;
    std::vector<std::shared_ptr<Event>> _heap;
};