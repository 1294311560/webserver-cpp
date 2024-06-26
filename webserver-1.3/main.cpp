#include <sys/timerfd.h>
#include <iostream>
#include <ostream>
#include <string.h>
#include <thread>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Timer.h"
#include "Acceptor.h"
#include "SocketsOps.h"

void newConnection(int sockfd, const InetAddress &peerAddr)
{
    printf("newConnection(): accepted a new connection from %s\n", peerAddr.toHostPort().c_str());
    ::write(sockfd, "How are you?\n", 13);
    sockets::close(sockfd);
}

int main()
{   
    printf("main(): pid = %d\n", getpid());
    
    InetAddress listenAddr(9981);
    EventLoop loop;
    
    Acceptor acceptor(&loop, listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();
    
    loop.loop();

}
