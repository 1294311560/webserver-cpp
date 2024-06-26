#include "TcpServer.h"

#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf


TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    name_(listenAddr.toHostPort()),
    acceptor_(new Acceptor(loop, listenAddr)),
    threadPool_(new EventLoopThreadPool(loop)),
    started_(false),
    nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  std::cout << "~TcpServer()" << std::endl;
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
    threadPool_->start();
  }

  if (!acceptor_->listenning())
  {
    loop_->runInLoop(
    boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    std::cout << "TcpServer::newConnection [" << name_
              << "] - new connection [" << connName
              << "] from " << peerAddr.toHostPort() << std::endl;
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    EventLoop* ioLoop = threadPool_->getNextLoop();
    std::cout<<"new connection thread id"<< ioLoop->getThreadId() << std::endl;
    TcpConnectionPtr conn(
        new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1)
    );
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  std::cout << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name() << std::endl;
  size_t n = connections_.erase(conn->name());
  assert(n == 1); (void)n;
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
}