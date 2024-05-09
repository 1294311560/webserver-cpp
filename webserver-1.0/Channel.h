#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>
#include <memory>

class EventLoop;
// Reactor最核心的事件分发机制
// Channel Class封装关心的文件描述符、关心的事件和对应的处理函数。
// class Channel的功能主要在于创建事件pollfd以及相应的回调函数
// 该对象的实例只能在所属的EventLoop中被调用，所以不需要考虑线程安全问题。
class Channel {
public:
    typedef std::function<void ()> EventCallback;

    Channel(EventLoop *loop, int fd);

    void handleEvent();

    void setReadCallback(const EventCallback &cb) {
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback &cb) {
        writeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback &cb) {
        errorCallback_ = cb;
    }

    int fd() const {
        return fd_;
    }

    int events() const {
        return events_;
    }
    void setRevents(int revt) {
        revents_ = revt;
    }

    bool isNoneEvent() const 
    {
        return events_ == kNoneEvent;
    }

    void enableReading() {
        events_ |= kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }
    int index() {
        return index_;
    }
    void setIndex(int idx) {
        index_ = idx;
    }
    EventLoop* ownerloop() {
        return loop_;
    }
private:
    void update();  // 调用EventLoop的update()，然后再去调用Poller Class的update()

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;
    int events_;   // events_关心的IO事件，由用户设置
    int revents_;   // 目前活动的事件，由EventLoop/Poller设置
    int index_;     // used by Poller.

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
};
#endif