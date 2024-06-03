#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <functional>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include "TimerQueue.h"
#include "Timer.h"
// EventLoop是不可拷贝的
// EventLoop的基本结构, 不涉及具体的事件处理机制

class Channel;
class Poller;
// class TimerQueue;

class EventLoop : boost::noncopyable {
public: 
    typedef std::vector<Channel*> ChannelList;
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();

    void loop();

    void quit() {
        quit_ = true;
        if(!isInLoopThread()) {
            wakeup();
        }
    }
    void wakeup();
    void handleRead();
     // 判断当前对象是否在本线程调用
    bool isInLoopThread() {
        true;
        //TODO
        return threadId_ == std::this_thread::get_id();  // 暂时还未实现
    }
    void assertInLoopThread() {
        if (!isInLoopThread())
        {
            std::cout<<"assertInLoopThread is not InLoopThread"<<std::endl;
            abortNotInLoopThread();
        }
    }
    void updateChannel(Channel *channel);
    void removeChannel(Channel* channel);

    EventLoop* getEventLoopOfCurrentThread();

    TimerId runAt(const Timestamp &time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);

    // 如果用户在当前IO线程调用这个函数，回调会同步进行；如果用户在其他线程调用runInLoop()，
    // cb会被加入队列，IO线程会被唤醒来调用这个Functor。
    // 有了这个功能，我们就能轻易地在线程间调配任务，
    // 比方说把TimerQueue的成员函数调用移到其IO线程，这样可以在不用锁的情况下保证线程安全性。
    void runInLoop(const Functor &cb);
    void queueInLoop(const Functor &cb);

private:
    void abortNotInLoopThread(){
        assert(true);
    };
    void doPendingFunctors();
    

    bool looping_;
    bool callingPendingFunctors_;
    bool quit_; 
    const std::thread::id threadId_;
    Timestamp pollREturnTime_;
    std::unique_ptr<Poller> poller_;
    ChannelList activeChannels_;
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
    Channel* currentActiveChannel_;
 };

 #endif