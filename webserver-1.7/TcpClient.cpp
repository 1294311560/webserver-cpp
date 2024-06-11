#include "TcpClient.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>
#include <iostream>

void removeConnection_(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector) {

}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  connector_->setNewConnectionCallback(
      boost::bind(&TcpClient::newConnection, this, _1));
  // FIXME setConnectFailedCallback
  std::cout << "TcpClient::TcpClient[" << this
           << "] - connector " << get_pointer(connector_) << std::endl;
}

TcpClient::~TcpClient() {
    std::cout << "TcpClient::~TcpClient[" << this
           << "] - connector " << get_pointer(connector_) << std::endl;
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conn = connection_;
    }
    if(conn) {
        CloseCallback cb = boost::bind(&removeConnection_, loop_, _1);
        loop_->runInLoop(
            boost::bind(&TcpConnection::setCloseCallback, conn, cb));
    }
    else {
        connector_->stop();
        loop_->runAfter(1, boost::bind(&removeConnector, connector_));
    }
}
void TcpClient::connect() {
    std::cout<< " TcpClient::connect() " << std::endl;
    connect_ = true;
    connector_->start();
}
void TcpClient::disconnect() {
    connect_ = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(connection_) {
            connection_->shutdown();
        }
    }
}
void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}
void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[23];
    ++nextConnId_;
    std::string connName = buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(
        loop_,
        connName,
        sockfd,
        localAddr,
        peerAddr
    ));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        boost::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}
void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        std::cout << "TcpClient::connect[" << this << "] - Reconnecting to "
                << connector_->serverAddress().toHostPort() << std::endl;
        connector_->restart();
  }
}