#pragma once
#include "Event.hpp"
#include <functional>
#include <vector>
#include <memory>
#include <iostream>
//模板最小堆类，要求T类内部重载比较运算符
template<typename T>
class MinHeap {
  public:
    //设定初始大小
    MinHeap(){};
    MinHeap(MinHeap<T>& minheap) { _heap(minheap); }
    MinHeap(MinHeap<T>&& minheap) { _heap(std::move(minheap)); }
    //添加元素
    void addItem(T& item) {
        int hole = getSize(); //新增元素在数组中的位置
        _heap.push_back(item);
        percolateUp(hole);
    }
    //删除元素
    //void eraseItem(std::shared_ptr<T> item) {}
    //获取最小元素
    T getMin() const {
        return _heap[0];
    }
    //删除最小元素
    void popMin() {
        if(isEmpty()) return;
        //将根节点的元素与最后一个元素互换，再将当前的根节点下沉即可
        std::swap(_heap[0], _heap[getSize() - 1]);
        _heap.pop_back(); // vector删除尾元素的时间复杂度是1
        if (!isEmpty()) {
            percolateDown(0);
        }
        
    }
    //是否为空
    inline bool isEmpty() { return _heap.empty(); }
    //最小堆大小
    inline int getSize() { return _heap.size(); }

    
  private:
    //将第hole个元素下虑，确保以第hole个结点作为根的子树拥有最小堆性质
    void percolateDown(int hole) {
        int child = 0;
        for (; (hole * 2 + 1) < getSize(); hole = child) {
            //左子节点
            child = hole * 2 + 1;
            //挑选一个较小的节点进行交换
            if (child < getSize() - 1 && (_heap[child + 1]) < (_heap[child])) {
                //此时右节点较小，切换到右节点
                child++;
            }
            if (_heap[child] < _heap[hole]) {
                std::swap(_heap[hole], _heap[child]);
            }
            else {
                break;
            }
        }
    }
    //将第hole个元素上浮
    void percolateUp(int hole) {
        int parent = 0;
        T tmp = _heap[hole];
        //上浮操作
        for (; hole > 0; hole = parent) {
            parent = (hole - 1) / 2;
            if (_heap[parent] > tmp) {
                //这里先不急着交换，在parent位置留下一个空穴
                _heap[hole] = _heap[parent];
            }
            else {
                break;
            }
        }
        _heap[hole] = tmp;
    }
  private:
    std::vector<T> _heap;
    
};