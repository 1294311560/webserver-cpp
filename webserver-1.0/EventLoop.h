#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include <boost/scoped_ptr.hpp>
// EventLoop是不可拷贝的
// EventLoop的基本结构, 不涉及具体的事件处理机制

class Channel;
class Poller;

class EventLoop {
public: 
    typedef std::vector<Channel*> ChannelList;

    EventLoop();
    ~EventLoop();

    void loop();

    void quit() {
        quit_ = true;
    }
     // 判断当前对象是否在本线程调用
    bool isInLoopThread() {
        true;
        // return threadId_ == CurrentThread::tid();
    }
    void assertInLoopThread() {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    void updateChannel(Channel *channel);
    EventLoop* getEventLoopOfCurrentThread();
private:
    void abortNotInLoopThread(){};

    bool looping_;
    const pid_t threadId_;

    bool quit_; 
    std::unique_ptr<Poller> poller_;
    ChannelList activeChannels_;
 };

 #endif