#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <map>
#include "TcpConnection.h"

class TcpServer {
public:
    TcpServer(EventLoop* loop, const InetAddress &listenAddr);
    ~TcpServer();
    void start();
     /// Set connection callback.
    /// Not thread sage.
    void setConnectionCallback(const ConnectionCallback &cb)
    { connectionCallback_ = cb; }
    
    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const MessageCallback &cb)
    { messageCallback_ = cb; }

    void removeConnection(const TcpConnectionPtr &conn);
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string name_;
    boost::scoped_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};

#endif 