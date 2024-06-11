#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <condition_variable>
#include <mutex>
#include <thread>

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "EventLoop.h"
#include "EventLoopThread.h"

class EventLoopThreadPool {
public:
    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) {
        numThreads_ = numThreads;
    }
    void start();
    EventLoop* getNextLoop();
private:
    EventLoop *baseLoop_;
    bool started_;
    int numThreads_;
    int next_;
    // boost::ptr_vector<T>类似std::vector<std::unique_ptr<T>>
    // 当threads_销毁时，其中的元素指向的对象也会被销毁
    boost::ptr_vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;
};

#endif