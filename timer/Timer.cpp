#include "Timer.h"
#include "Log.h"

#include <bits/stdint-uintn.h>
#include <bits/types/struct_timespec.h>
#include <ctime>
#include <memory>
#include <mysql/mysql.h>
#include <type_traits>

//添加定时器
void Timer::addTimer(std::shared_ptr<Event> e) {
    int hole = getSize(); //新增元素在数组中的位置
    e->_pos = hole;
    _heap.push_back(e);
    percolateUp(hole);
}
//删除目标定时器
//根据Event类中的pos找到目标定时器的位置，将其与堆内最后一个元素互换位置，并把最后一个元素pop出去
// 被交换的元素需要根据其与父节点的关系选择上升或者下沉
//
void Timer::delTimer(std::shared_ptr<Event> e) {
    if(e == nullptr) return;
    int pos = e->_pos;
    if (pos >= getSize())
        return;
    std::swap(_heap[pos], _heap[getSize() - 1]);
    _heap[pos]->_pos = pos;
    _heap.pop_back(); //将该节点移出定时器

    int parent = (pos - 1) / 2;
    //如果该节点不是根节点且比父节点小，上浮
    if (pos > 0 && *_heap[parent] > *_heap[pos]) {
        percolateUp(pos);
    }
    else {
        //反之，一直下沉到合适的位置
        percolateDown(pos);
    }
}
//获得顶部定时器
std::shared_ptr<Event> Timer::getTop() {
    if (isEmpty())
        return nullptr;
    return _heap[0];
}
//删除顶部定时器
void Timer::popTimer() {
    if (isEmpty())
        return;
    //将根节点的元素与最后一个元素互换，再将当前的根节点下沉即可
    std::swap(_heap[0], _heap[getSize() - 1]);
    _heap[0]->_pos = 0;
    _heap.pop_back(); // vector删除尾元素的时间复杂度是1
    if (!isEmpty()) {
        percolateDown(0);
    }
}
//修改目标定时器
//如果目标定时器不存在，将其插入堆中
void Timer::adjustTimer(std::shared_ptr<Event> e) {
    if (e->_pos > getSize() - 1) {
        addTimer(e);
        return;
    }
    int pos = e->_pos;
    _heap[pos] = e;
    //修改定时器的方法与删除定时器类似，根据与父节点的关系判断是否要上浮或者下沉
    int parent = (pos - 1) / 2;
    if (pos > 0 && *_heap[parent] > *_heap[pos]) {
        percolateUp(pos);
    }
    else {
        percolateDown(pos);
    }
}

//将第hole个元素下虑，确保以第hole个结点作为根的子树拥有最小堆性质
//同时更新沿途各个事件的pos
void Timer::percolateDown(int hole) {
    int child = 0;
    for (; (hole * 2 + 1) < getSize(); hole = child) {
        //左子节点
        child = hole * 2 + 1;
        //挑选一个较小的节点进行交换
        if (child < getSize() - 1 && *(_heap[child + 1]) < *(_heap[child])) {
            //此时右节点较小，切换到右节点
            child++;
        }
        if (*_heap[child] < *_heap[hole]) {
            std::swap(_heap[hole], _heap[child]);
            //更新自身位置
            _heap[hole]->_pos = hole;
            _heap[child]->_pos = child;
        }
        else {
            break;
        }
    }
}
//将第hole个元素上浮
void Timer::percolateUp(int hole) {
    int parent = 0;
    auto tmp = _heap[hole];
    //上浮操作
    for (; hole > 0; hole = parent) {
        parent = (hole - 1) / 2;
        if (*_heap[parent] > *tmp) {
            //这里先不急着交换，在parent位置留下一个空穴
            _heap[hole] = _heap[parent];
            _heap[hole]->_pos = hole; //更新位置
        }
        else {
            break;
        }
    }
    _heap[hole] = tmp;
    _heap[hole]->_pos = hole;
}

//延长某个定时器的时间
//延长为当前时间+time
void Timer::extendTime(std::shared_ptr<Event> e, std::uint32_t time) {
    timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    time = time + spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    e->_time = time;
    adjustTimer(e);
}

//获取当前时间，调用过期事件的回调函数，并把事件删除
void Timer::tick() {
    if (isEmpty())
        return;
    //获取系统启动以来经过的时间
    timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    //当前事件单位是毫秒
    uint32_t cur = spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
    while (!isEmpty()) {
        std::shared_ptr<Event> tmp = getTop();
        //如果最小的时间都比当前事件大，则没有必要继续处理
        if (tmp->_time > cur) {
            break;
        }
        //如果事件到期，调用回调函数并删除该元素
        else {
            printf("Event timeout\n");
            LOG_DEBUG(__FUNCTION__, "%s", "Event timeout");
            tmp->callBack();
            popTimer();
        }
    }
}