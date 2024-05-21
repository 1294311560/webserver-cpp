#include "TimerQueue.h"
#include "EventLoop.h"
#include <sys/timerfd.h>
#include <string.h>

//创建一个ie定时器对象并返回定时器对象描述符
int createTimerfd() {
    // timerfd_create 是一个系统调用函数，用于创建一个新的定时器对象，
    // 并返回一个文件描述符，该描述符可以用于读取定时器的事件。
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    assert(timerfd >= 0);

    return timerfd;
}
// 给定的时间距离当前的时间
struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if(microseconds < 100)
        microseconds = 100;

    // timespec 这个结构体用于表示一个时间点或者持续时间。它有两个成员变量：
    // time_t tv_sec: 这是一个整数类型的变量，表示秒数，用于存储时间的整数部分。
    // long tv_nsec: 这也是一个整数类型的变量，表示纳秒数，用于存储时间的小数部分。
    struct timespec ts;

    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts; 
}
// read 从定时器文件描述符读取到期次数，存储在 howmany 中。
// 如果读取的字节数与 sizeof(howmany) 不符，表示读取出错。
void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    std::cout<<"TimerQueue::handleRead() " << howmany << " at " << now.toString()<<std::endl;
    if(n != sizeof howmany) {
        std::cout<< "TimerQueue::handleRead() reads " << n << " bytes instead of 8"<<std::endl;
    }
}
// 重新设置timerfd_超时时间为expiration
void resetTimerfd(int timerfd, Timestamp expiration) {
    // struct itimerspec:
    // 这个结构体用于描述一个定时器的初始值和间隔值。它有两个成员变量：
    // struct timespec it_interval: 这是一个 struct timespec 类型的变量，用于指定定时器的间隔值。如果定时器只需要触发一次，则该值可以设置为 0。
    // struct timespec it_value: 这也是一个 struct timespec 类型的变量，用于指定定时器的初始值，即第一次触发的时间。
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration); // 设置新的定时器到期时间。
    // 设置一个基于文件描述符的定时器
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        std::cout << "timerfd_settime()"<<std::endl;
    }
}
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
     timerfd_(createTimerfd()),
     timerfdChannel_(loop, timerfd_), 
     timers_(),
     callingExpiredTimers_(false) 
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    // 执行所有过期定时器的回调函数
    for(const Entry& it : expired) {
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

TimerQueue::~TimerQueue(){
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(const Entry& timer : timers_) {
        delete timer.second;
    }
}
// 从timers_中移除已到期的Timer，并通过vector返回它们
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;

    Entry sentry = std::make_pair(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    // sentry让set::lower_bound()返回的是第一个未到期的Timer的迭代器
    // lower_bound方法会找到第一个大于等于sentry的值,即第一个未到期的Timer
    // pair的大小比较方法是先比较第一个元素，如果第一个元素相等，则比较第二个元素
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);

    // 把timers_的开头到it（不含it）的内容复制到expired的尾后,即所有已经到期的定时器
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);
    
    return expired;
}

TimerId TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, double interval) {
    Timer *timer = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();

    bool earlistestChanged = insert(timer);
    // 如果是最早的超时时间，设置定时器的超时时间
    if(earlistestChanged)
        resetTimerfd(timerfd_, timer->expiration());
}

void TimerQueue::cancel(TimerId timerid) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerid));
}
// 取消特定的定时器
void TimerQueue::cancelInLoop(TimerId timerid) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerid.timer_, timerid.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    // 去已激活定时器集合里找该定时器，如果找到，则把集合中该定时器删除。
    if(it != activeTimers_.end()) {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        delete it->first;
        activeTimers_.erase(it);
    }
    else if(callingExpiredTimers_) {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    Timestamp nextExpire;
    for(const Entry& it : expired) {
        ActiveTimer timer(it.second, it.second->sequence());
        // 如果已过期的定时器允许重复，并且该定时器未被注销，则更新定时器超时时间
        // 再把它插入队列中
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            it.second->restart(now);
            insert(it.second);
        }
        else { //不允许重复或者该定时器已经被注销，则销毁
            delete it.second;
        }
    }
    //更新文件描述符超时时间
    if(!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration(); // 下一次超时时间
    }
    if(nextExpire.valid()) {
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    // 如果集合中没有定时器，或者当前时间比集合中最早的时间还要早
    // 则当前时间为最早超时时间
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result
        = timers_.insert(Entry(when, timer));
        assert(result.second); 
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
        = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); 
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}