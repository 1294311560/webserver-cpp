#include <threads.h>
#include <pthread.h>
#include <assert.h>
#include <poll.h>
#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
// __thread修饰的变量，在多线程中不是共享的而是每个线程单独一个
__thread EventLoop *t_loopInThisThread = 0;

// one loop per thread顾名思义每个线程只能有一个EventLoop对象
// EventLoop的构造函数会检查当前线程是否已经创建了其他EventLoop对象，遇到错误就终止程序（LOG_FATAL）
// EventLoop的构造函数会记住本对象所属的线程(threadId_)
EventLoop::EventLoop() : looping_(false), quit_(false), threadId_(1), poller_(new Poller(this)){
    Poller loop = Poller(this);

    // LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread) {
        // LOG_FATAL << "Another EventLoop " << t_loopInThisThread 
        //           << " exists in this thread " << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop() {
    assert(!looping_);
    t_loopInThisThread = nullptr;
}

// 调用Poller::poll()获得当前活动事件的Channel列表，
// 然后依次调用每个Channel的handleEvent()函数：
void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while(!quit_) {
        activeChannels_.clear();
        poller_->poll(5, &activeChannels_);
        for(ChannelList::iterator it = activeChannels_.begin(); it != activeChannels_.end(); it++) {
            (*it)->handleEvent();
        }
    }

    looping_ = false;
}
void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerloop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}
EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}