#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "SocketsOps.h"
#include "Callback.h"
typedef struct sockaddr SA;

const SA* sockaddr_cast(const struct sockaddr_in* addr)
{
  return static_cast<const SA*>(implicit_cast<const void*>(addr));
}

SA* sockaddr_cast(struct sockaddr_in* addr)
{
    return static_cast<SA*>(implicit_cast<void*>(addr));
}

void setNonBlockAndCloseOnExec(int sockfd)
{
    // non-block
    // F_GETFL 命令用于获取文件描述符的状态标志（文件状态标志），
    // 这些标志包括但不限于 O_RDONLY、O_WRONLY、O_RDWR、O_NONBLOCK 等。
    int flags = ::fcntl(sockfd, F_GETFL, 0);

    // flags |= O_NONBLOCK; 这行代码将文件状态标志中添加 O_NONBLOCK 标志，表示将套接字设置为非阻塞模式。
    // 在非阻塞模式下，I/O 操作不会阻塞进程，而是立即返回。
    // ::fcntl(sockfd, F_SETFL, flags); 使用 F_SETFL 命令将修改后的标志重新设置到文件描述符中。
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec
    // F_GETFD 命令用于获取文件描述符标志。这些标志包括 FD_CLOEXEC，用于在执行 exec 系列函数时自动关闭文件描述符。
    flags = ::fcntl(sockfd, F_GETFD, 0);
    // 设置 FD_CLOEXEC 可以确保在调用 exec 系列函数执行新程序时，这个文件描述符会自动关闭，以防止文件描述符泄漏到子进程。
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    // FIXME check
}


int sockets::createNonblockingOrDie()
{
  // socket
#if VALGRIND
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
    std::cout << "createNonblockingOrDie";
    }

    setNonBlockAndCloseOnExec(sockfd);
#else
    // AF_INET 表示 IPv4 地址族
    // OCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC: 这个参数指定了套接字的类型和属性。
    // SOCK_STREAM: 表示这是一个面向连接的套接字，即 TCP 套接字。
    // SOCK_NONBLOCK: 这个标志指定了套接字为非阻塞模式。在非阻塞模式下，套接字的 I/O 操作（如读取或写入）不会阻塞进程，而是立即返回。
    // 这样可以避免在等待 I/O 完成时阻塞程序的执行。
    // SOCK_CLOEXEC: 这个标志指定了套接字在执行时会自动关闭（close-on-exec）。
    // 这意味着当进程调用 exec 系列函数执行新程序时，套接字会自动关闭，以防止文件描述符泄漏到子进程。
    int sockfd = ::socket(AF_INET,
                        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
    if (sockfd < 0)
    {
        std::cout << "createNonblockingOrDie" <<std::endl;
    }
#endif
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr)
{
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof addr);
    if (ret < 0)
    {
    std::cout << "bindOrDie" <<std::endl;
    }
}

void sockets::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0)
    {
    std::cout << "listenOrDie" <<std::endl;
    }
}

int sockets::accept(int sockfd, struct sockaddr_in* addr)
{
    socklen_t addrlen = sizeof *addr;
#if VALGRIND
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                            &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0)
    {
        int savedErrno = errno;
        std::cout << "Socket::accept"<<std::endl;
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
            // expected errors
            errno = savedErrno;
            break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
            // unexpected errors
            std::cout << "unexpected error of ::accept " << savedErrno <<std::endl;
            break;
            default:
            std::cout << "unknown error of ::accept " << savedErrno<<std::endl;
            break;
        }
    }
    return connfd;
}

void sockets::close(int sockfd)
{
    std::cout <<"sockets::close" <<std::endl;
    if (::close(sockfd) < 0)
    {
        std::cout <<"close" <<std::endl;
    }
}

void sockets::toHostPort(char* buf, size_t size,
                         const struct sockaddr_in& addr)
{
    char host[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromHostPort(const char* ip, uint16_t port,
                           struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = sockets::hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        std::cout << "fromHostPort"<<std::endl;
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = sizeof(localaddr);
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    {
    std::cout << "sockets::getLocalAddr" << std::endl;
    }
    return localaddr;
}
int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = sizeof optval;

    if(::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}