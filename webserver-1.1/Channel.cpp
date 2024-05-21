#include "Channel.h"
#include <poll.h>
#include "EventLoop.h"
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd) :loop_(loop), fd_(fd), events_(0), index_(-1) {

}

void Channel::update() {
    loop_->updateChannel(this); //Channel::update()会调用EventLoop::updateChannel()，后者会转而调用Poller::updateChannel()
}
#include <iostream>
// Channel::handleEvent()是Channel的核心，它由EventLoop::loop()调用，
// 它的功能是根据revents_的值分别调用不同的用户回调
void Channel::handleEvent() {
    std::cout<<"Channel::handle_event()"<<std::endl;
    if(revents_ & POLLIN) {
        // LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    // POLLNVAL表示fd未打开
    // POLLERR表示出错。它也会出现在管道读端关闭时，在管道的写端上设置
    if(revents_ & (POLLERR | POLLNVAL)) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    // POLLRDHUP在Linux 2.6.17被引入，流套接字的对端关闭了连接或writing half of connection
    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if(readCallback_) {
            readCallback_();
        }
    }

    if(revents_ & POLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}
void Channel::remove()
{
  assert(isNoneEvent());
//   addedToLoop_ = false;
//   loop_->removeChannel(this);
}