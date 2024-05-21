#include <sys/timerfd.h>
#include <iostream>
#include <ostream>
#include <string.h>
#include <thread>

#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Timer.h"
EventLoop *g_loop;

void print() { 
    std::cout<< "print()" <<std::endl;
}    // 空函数

void threadFunc()
{
    g_loop->runAfter(1.0, print);
}

int main()
{
    EventLoop loop;
    g_loop = &loop;
    
    std::thread t(threadFunc);
    std::cout<<"main Thread is running..."<<std::endl;
	
	t.join();
	std::cout<<" exit from main Thread"<<std::endl;

    loop.loop();
}
