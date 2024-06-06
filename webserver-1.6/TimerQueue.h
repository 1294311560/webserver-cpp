#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H
#include <vector>
#include <set>
#include <functional>
#include "Channel.h"
#include "Timestamp.h"
#include "TimerId.h"

class EventLoop;
class Timer;
// class TimerId;

class TimerQueue {
public:
    typedef std::function<void()> TimerCallback;

    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();
    TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);
    void cancel(TimerId timerid);
private:
    typedef std::pair<Timestamp, Timer*> Entry; // 以时间戳作为键值获取定时器
    typedef std::set<Entry> TimerList; // 底层使用红黑树管理，按照时间戳排序
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer); // 在本loop中添加定时器，线程安全
    void handleRead(); // 定时器读事件触发函数
    void cancelInLoop(TimerId timerid);

    std::vector<Entry> getExpired(Timestamp now); // 获取到期定时器
    void reset(const std::vector<Entry> &expired, Timestamp now); // 重置这些到期定时器
    bool insert(Timer* timer); // 插入定时器

    EventLoop *loop_; // 所属的Eentloop
    const int timerfd_; // timerfd是linux提供的定时器接口
    Channel timerfdChannel_; // 封装timerfd_文件描述符
    TimerList timers_; // 定时器队列（红黑树）
    ActiveTimerSet activeTimers_;
    ActiveTimerSet cancelingTimers_;
    bool callingExpiredTimers_; // 正在获取超时定时器
};
#endif