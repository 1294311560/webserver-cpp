# webserver-1.1

 **Timestamp类**
 
这个类用来表示一个时间戳  

在这个类中我们主要要完成两个内容，返回当前时间的时间戳time_t类型，并将其转化成字符串类型。  
  
底层成员变量是一个int64_t位的记录事件的整数microSecondsSinceRpoch_。  
   
   
Timestamp::now()显示当前时间

 定义一个整数类型time_t由time(NULL)返回当前时间，以秒为单位。同时返回Timestamp对象。time函数 ：返回自 Epoch（1970年1月1日 00:00:00 UTC）以来经过的秒数   
 
Timestamp::toString()格式转换  


**Timer类**

Timer类包含了一个超时时间戳和一个回调函数。当超时时间戳到达时，调用回调函数触发定时事件  

* 定时器到期后需要回调函数；
* 定时器需要记录我们的超时时间；
* 如果是重复事件（比如每间隔5秒扫描一次用户连接），我们还需要记录超时间间隔；
* 对应的，我们需要一个bool类型的值标注这个定时器是一次性的还是重复的。

如果是需要重新利用的定时器，会调用restart方法，我们设置其下一次超时时间为「当前时间 + 间隔时间」。如果是「一次性定时器」，那么就会自动生成一个空的 Timestamp，其时间自动设置为 0.0

**TimeQueue类**

TimerQueue类是一个基于事件戳排序的定时器容器。它使用了最小堆（MinHeap）数据结构来保证定时器按照超时时间的顺序进行排列。TimerQueue类提供了添加、删除和获取最近超时的定时器的接口。

* 整个TimerQueue只打开一个timefd，用以观察定时器队列队首的到期事件。其原因是因为set容器是一个有序队列，以升序排序，就是说整个队列中，Timer的到期时间时从小到大排列的，正是因为这样，才能做到节省系统资源的目的。  
* 整个定时器队列采用了muduo典型的事件分发机制，可以使得定时器的到期时间像fd一样在Loop线程中处理。

TimerQueue使用了一个Channel来观察timerfd_上的readable事件  
 
getExpired()函数会从timers_中移除已到期的Timer，并通过vector返回它们

### 执行流程

EventLoop::loop() -> Poller::poll() -> poll()  

EventLoop::runAfter() -> Timestamp::addTime(Timestamp::now(), delay) ->runAt() -> timerQueue_->addTimer()

1. 创建EventLoop结构体，EventLoop构造函数会创建
   * Poller
   * TimerQueue
   * Channel
    
   EventLoop::runInLoop()函数用来确保在属于自己的IO线程内执行某个用户任务回调，如果用户在当前IO线程调用这个函数，回调会同步进行；如果用户在其他线程调用runInLoop()，cb会被加入队列，IO线程会被唤醒来调用这个Functor，这样可以在不用锁的情况下保证线程安全性。

2. TimerQueue构造函数
   createTimerfd（） 创建一个定时器文件描述符 timerfd_    
   创建一个Channel： timerfdChannel_  
   TimerQueue::handleRead() 超时后的回调函数，会调用getExpired（）函数获得到期的Timer,并执行Timer的回调函数

3. Channel_
   .setReadCallback函数用来设置回调函数  
   .enableReading()最终会调用 Poller::updateChannel(Channel *channel)，将channel对应的文件描述符放到数组中,调用poll对该数组进行监听，当有事件发生时，调用fillActiveChannels将发生事件的channel放入活动列表中，然后在EventLoop::loop()函数中会遍历活动列表，通过Channel::handleEvent()调用他们的回调函数


   ![1716293539156.jpg](./1716293539156.jpg)