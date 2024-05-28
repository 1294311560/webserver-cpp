#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include "Socket.h"
#include "SocketsOps.h"

Socket::~Socket() {
    sockets::close(sockfd_);
}
void Socket::bindAddress(const InetAddress& addr)
{
  sockets::bindOrDie(sockfd_, addr.getSockAddrInet());
}

void Socket::listen()
{
  sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
  struct sockaddr_in addr;
  bzero(&addr, sizeof addr);
  int connfd = sockets::accept(sockfd_, &addr);
  if (connfd >= 0)
  {
    peeraddr->setSockAddrInet(addr);
  }
  return connfd;
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  //允许在套接字关闭后立即重新使用先前使用过的本地地址和端口号
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof optval);
  // FIXME CHECK
}