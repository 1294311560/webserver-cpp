#ifndef CALLBACK_H
#define CALLBACK_H
#include "Buffer.h"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
// template<typename T, typename U>
// T implicit_cast(U const &u) {
//     return u;
// }
template<typename To, typename From>
inline To implicit_cast(From const &f)
{
  return f;
}
class TcpConnection;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::function<void()> TimerCallback;
typedef boost::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void (const TcpConnectionPtr &, Buffer *buf, Timestamp)> MessageCallback;

typedef boost::function<void (const TcpConnectionPtr&)> CloseCallback;
#endif