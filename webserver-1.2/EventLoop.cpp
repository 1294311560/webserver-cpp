#include <threads.h>
#include <pthread.h>
#include <assert.h>
// #include <socket.h>
#include <poll.h>
#include <algorithm>

#include <signal.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <sys/uio.h>

#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
// __thread修饰的变量，在多线程中不是共享的而是每个线程单独一个
__thread EventLoop *t_loopInThisThread = 0;

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    assert(evtfd >= 0);
    return evtfd;
}

// one loop per thread顾名思义每个线程只能有一个EventLoop对象
// EventLoop的构造函数会检查当前线程是否已经创建了其他EventLoop对象，遇到错误就终止程序（LOG_FATAL）
// EventLoop的构造函数会记住本对象所属的线程(threadId_)
EventLoop::EventLoop() 
    : looping_(false), 
     quit_(false), 
     threadId_(std::this_thread::get_id()), 
     poller_(new Poller(this)),
     callingPendingFunctors_(false),
     timerQueue_(new TimerQueue(this)),
     wakeupFd_(createEventfd()),
     wakeupChannel_(new Channel(this, wakeupFd_)),
     currentActiveChannel_(NULL)
{
    // LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread) {
        // LOG_FATAL << "Another EventLoop " << t_loopInThisThread 
        //           << " exists in this thread " << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
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
        doPendingFunctors();
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
TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb) {
    return timerQueue_->addTimer(cb, time, 0.0);
}
TimerId EventLoop::runAfter(double delay,  const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}
TimerId EventLoop::runEvery(double interval, const TimerCallback &cb) {
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, 0.0);
}

void EventLoop::runInLoop(const Functor &cb) {
    if(isInLoopThread()) {
        cb();
    }
    else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    mutex_.lock();
    pendingFunctors_.push_back(cb);
    mutex_.unlock();

    if(!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    mutex_.lock();
    functors.swap(pendingFunctors_);
    for(size_t i = 0; i < functors.size(); i++) {
        functors[i]();
    } 
    callingPendingFunctors_ = false;
}
void EventLoop::wakeup()
{
    std::cout << "EventLoop::wakeup() "<< std::endl;
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8"<< std::endl;
  }
}

void EventLoop::handleRead() {
    std::cout << "EventLoop::handleRead() "<< std::endl;
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        printf("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}