# webserver-1.2

实现EentLoopThread类

EventLoopThread提供了对应eventloop和thread的封装，EventLoopThread可以创建一个IO线程,并执行loop

startLoop函数会返回新线程中EventLoop对象的地址，因此用条件变量来等待线程的创建与运行

## 执行流程

EventLoopThread::EventLoopThread() 构造函数会创建一个I/O线程

EventLoopThread::startLoop()等待线程执行创建EventLoop，创建完毕后会调用loop等待事件发生

在main线程中执行loop->runInLoop(threadFunc);会调用EventLoop::wakeup()唤醒I/O线程执行回调函数

EventLoop::wakeup()会触法事件调用对应Channel的handleEvent()，然后调用EventLoop::handleRead()

EventLoop::doPendingFunctors()从队列中取出threadFunc函数执行