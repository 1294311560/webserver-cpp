#include "Connector.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>
#include <errno.h>

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr) 
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
    std::cout<<"ctor[" << this << "]"<<std::endl;
}
Connector::~Connector()
{
    std::cout<<"dtor[" << this << "]"<<std::endl;
    loop_->cancel(timerId_);
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(boost::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}
void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        std::cout<< "do not connect" << std::endl;
    }
}
void Connector::connect() {
    int sockfd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
    int savedErrno = (ret == 0)? 0:errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            std::cout << "connect error in Connector::startInLoop " << savedErrno <<std::endl;
            sockets::close(sockfd);
            break;

        default:
            std::cout << "Unexpected error in Connector::startInLoop " << savedErrno <<std::endl;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->cancel(timerId_);
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(
        boost::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorCallback(
        boost::bind(&Connector::handleError, this)); // FIXME: unsafe

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();
}
int Connector::removeAndResetChannel() {
    channel_->disableAll();
    loop_->removeChannel(get_pointer(channel_));
    int sockfd = channel_->fd();
    loop_->queueInLoop(boost::bind(&Connector::resetChannel, this));
    return sockfd;
}
void Connector::resetChannel()
{
  channel_.reset();
}
void Connector::handleWrite() {
    std::cout<< "Connector::handleWrite()" <<std::endl;
    if(state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err) {
            std::cout << "Connector::handleWrite - SO_ERROR = "
               << err  << std::endl; 
        }
        else if (sockets::isSelfConnect(sockfd)) { //判断是否为自连接
            std::cout << "Connector::handleWrite - Self connect" << std::endl;
            retry(sockfd);
        }
        else {
            setState(kConnected);
            if(connect_) {
                newConnectionCallback_(sockfd);
            }
            else {
                sockets::close(sockfd);
            }
        }
    }
    else {
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError() {
    std::cout << "Connector::handleError" << std::endl;
    assert(state_ == kConnecting);

    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    std::cout << "SO_ERROR = " << err << " " << std::endl;
    retry(sockfd);
}

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_) {
       std::cout << "Connector::retry - Retry connecting to "
             << serverAddr_.toHostPort() << " in "
             << retryDelayMs_ << " milliseconds. " <<std::endl;
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0, 
                               boost::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else {
        std::cout << "do not connect" << std::endl;
    }
}