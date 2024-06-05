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
    socket_->setKeepAlive(true);
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
  std::cout<<"TcpConnection::handleWrite()"<<std::endl;
  loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (n > 0)
        {
          outputBuffer_.retrieve(n);
          if (outputBuffer_.readableBytes() == 0)
          {
              channel_->disableWriting();    // 1
              if(writeCompleteCallback_){
                loop_->queueInLoop(
                  boost::bind(writeCompleteCallback_, shared_from_this())
                );
              }
              if (state_ == kDisconnecting)    // 2
              {
                  shutdownInLoop();
              }
          }
          else
          {
              std::cout << "I am going to write more data" <<std::endl;
          }
      }
      else
      {
          std::cout << "TcpConnection::handleWrite" <<std::endl;
      }
  }
  else
  {
      std::cout << "Connecting is down, no more writing" <<std::endl;
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  std::cout<<"TcpConnection::handleClose state = " << state_ << std::endl;
  assert(state_ == kConnected || state_ == kDisconnecting);
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
  assert(state_ == kConnected || state_ == kDisconnecting);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(get_pointer(channel_));
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap（often abbreviated as CAS）
    // CAS指原子地进行此过程：查看某个变量的值，如果它不符合某个条件，则将其更新为新值
    // 此处指的应该是原子地查看state_的值，如果它等于kConnected，则将其更新为kDisconnecting
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::send(const std::string &message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
   // if no thing in output queue, try writing directly
  if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0){
      if (implicit_cast<size_t>(nwrote) < message.size())
      {
          std::cout << "I an going to write more data" <<std::endl;
      }
      else if(writeCompleteCallback_){
        loop_->queueInLoop(
          boost::bind(writeCompleteCallback_, shared_from_this())
        );
      }
    }
    else
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
          std::cout << "TcpConnection::sendInLoop" <<std::endl;
      }
    }
  }

  assert(nwrote >= 0);
  if(implicit_cast<size_t>(nwrote) < message.size()) {
    outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
    if (!channel_->isWriting())
    {
        channel_->enableWriting();
    }
  }
}

void TcpConnection::setTcpNoDelay(bool on) {
  socket_->setTcpNoDelay(on);
}