#ifndef POLLER_H
#define POLLER_H

#include <vector>
#include <map>
#include <boost/noncopyable.hpp>
#include "EventLoop.h"
#include "Timestamp.h"
class Channel;

class Poller {
public:
    typedef std::vector<Channel *> ChannelList;

    Poller(EventLoop *loop);
    ~Poller();

    // Polls the I/O events.
    // Must be called in the loop thread.
    Timestamp poll(int timeoutMs, ChannelList *activeChannels);
    // Changes the interested I/O events.
	// Must be called in the loop thread.
    void updateChannel(Channel *channel);

    void assertInLoopThread() {
        ownerLoop_->assertInLoopThread();
    }
private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel *> ChannelMap;

    EventLoop* ownerLoop_;
    PollFdList pollfds_;
    ChannelMap channels_;  //从fd到Channel *的映射
};

#endif