#include <sys/timerfd.h>
#include <iostream>
#include <ostream>
#include <string.h>
#include <thread>


#include "EventLoopThread.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Timer.h"
#include "SocketsOps.h"
#include "TcpServer.h"

void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        printf("onConnection(): new connection [%s] from %s\n",
            conn->name().c_str(),
            conn->peerAddress().toHostPort().c_str());
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
            conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr& conn,
            Buffer* buf,
            Timestamp receiveTime)
{
    printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toFormattedString().c_str());

    printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

int main()
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr(9981);
    EventLoop loop;

    TcpServer server(&loop, listenAddr);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    loop.loop();
}