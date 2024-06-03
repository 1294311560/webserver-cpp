#ifndef TIMER_H
#define TIMER_H

#include<functional>
#include "Timestamp.h"
#include "Callback.h"
class Timer {
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when), 
      interval_(interval),
      repeat_(interval > 0.0),
       sequence_(1){}

    void run() const {
        callback_();
    }

    Timestamp expiration() const {
        return expiration_;
    }

    bool repeat() const {
        return repeat_;
    }
    uint64_t sequence() const {
        return sequence_;
    }
    void restart(Timestamp now);
private:
    const TimerCallback callback_; //定时器回调函数
    Timestamp expiration_; // 下一次的超时时间
    const double interval_; // 超时时间间隔，如果是一次性定时器，该值为0
    const bool repeat_; // 是否重复，一次性定时器为false
    const int64_t sequence_;
    // static AtomicInt64 s_numCreated_;
};

#endif