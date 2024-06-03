#include "TcpConnection.h"
#include <boost/bind.hpp>

#include <errno.h>
#include <iostream>
#include "SocketsOps.h"

TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
    std::cout << "TcpConnection::ctor[" <<  name_ << "] at " << this
        << " fd=" << sockfd << std::endl;
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
  std::cout << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd() << std::endl;
}

void TcpConnection::connectEstablished()
{
   std::cout<<"TcpConnection::connectEstablished()"<<std::endl;
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  std::cout<<"TcpConnection::handleRead()"<<std::endl;
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if(n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0) {
    handleClose();
  }
  else {
    errno = savedErrno;
    handleError();
  }
}
void TcpConnection::handleWrite() {
  std::cout<<"TcpConnection::handleWrite" << std::endl;
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  std::cout<<"TcpConnection::handleClose state = " << state_ << std::endl;
  assert(state_ == kConnected);
  channel_->disableAll();
  closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    std::cout<< "TcpConnection::handleError [" << name_ 
              << "] - SO_ERROR = " << err << std::endl;
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  assert(state_ == kConnected);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(get_pointer(channel_));
}