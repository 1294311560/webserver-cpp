#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string.h>
#include "EventLoop.h"


class EventLoopThread {
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    EventLoopThread();
    // EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
    //                 const std::string& name = std::string());
    ~EventLoopThread(); 
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_; //线程是否退出
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    // ThreadInitCallback callback_;
};

#endif