#ifndef SOCKET_H
#define SOCKET_H
#include "InetAddress.h"
class Socket {
public:
    explicit Socket(int sockfd): sockfd_(sockfd) 
    {}
    ~Socket();

    int fd() {
        return sockfd_;
    }
    void bindAddress(const InetAddress& localaddr);
    void listen();

    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    int accept(InetAddress* peeraddr);
    void shutdownWrite();
    void setReuseAddr(bool on);
    void setTcpNoDelay(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};
#endif