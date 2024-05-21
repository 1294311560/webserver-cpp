#include <sys/timerfd.h>
#include <iostream>
#include <ostream>
#include <string.h>
#include <thread>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Timer.h"
EventLoop *g_loop;

void print() { 
    std::cout<< "print()" <<std::endl;
}    // 空函数

void threadFunc(EventLoop* loop)
{
    loop->runAfter(1.0, print);
}

int main()
{   
    // std::thread t(threadFunc);
    EventLoopThread t(&threadFunc, "thread");
    t.startLoop();

    std::cout<<"main Thread is running..."<<std::endl;
	
	std::cout<<" exit from main Thread"<<std::endl;
}
