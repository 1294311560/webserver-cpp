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

void threadFunc()
{
    printf("runInThread(): pid = %d, tid = %d\n",
         getpid(), gettid());
}

int main()
{   
    EventLoopThread t;
    EventLoop* loop = t.startLoop();
    printf("main thread : pid = %d, tid = %d\n",
         getpid(), gettid());
    loop->runInLoop(threadFunc);
    sleep(1);
    loop->runAfter(2, threadFunc);
    sleep(3);
    loop->quit();

    std::cout<<"main Thread is running..."<<std::endl;
	
	std::cout<<" exit from main Thread"<<std::endl;
}
