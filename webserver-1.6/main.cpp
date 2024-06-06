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

std::string message;
void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           gettid(),
           conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str());
    conn->send(message);
  }
  else
  {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           gettid(),
           conn->name().c_str());
  }
}

void onWriteComplete(const TcpConnectionPtr& conn)
{
  conn->send(message);
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         gettid(),
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toFormattedString().c_str());

  buf->retrieveAll();
}

int main(int argc, char* argv[])
{
  printf("main(): pid = %d, tid = %d\n", getpid(), gettid());
  
  std::string line;
  for (int i = 33; i < 127; ++i)
  {
    line.push_back(char(i));
  }
  line += line;

  for (size_t i = 0; i < 127-33; ++i)
  {
    message += line.substr(i, 72) + '\n';
  }

  InetAddress listenAddr(9981);
  EventLoop loop;

  TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.setWriteCompleteCallback(onWriteComplete);
  // if (argc > 1) {
  //   server.setThreadNum(atoi(argv[1]));
  // }
  server.setThreadNum(5);
  server.start();

  loop.loop();
}