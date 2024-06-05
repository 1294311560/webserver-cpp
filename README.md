# webserver-cpp

## webserver-1.0

    webserver-1.0只实现了Reactor的基础框架，作为webserver的入门

    webserver-1.0实现了三个类EventLoop、Poller、Channel，定义了一个定时器，监听该事件，当定时器到期时，会触发事件，并调用相应的回调函数处理事件。  

    webserver-1.0构成了Reactor模式的核心内容，作为webserver的基础框架


## webserver-1.1

    webserver-1.1在1.0的基础上实现了定时器功能。

    实现了三个类TimerQueue、Timestamp、Timer

    详见webserver-1.1/README


## webserver-1.2

    webserver-1.2在1.1的基础上实现了EventLoopThread类，这个类封装了线程和EentLoop的创建过程  

    到目前为止，Reactor事件处理框架已初具规模，接下来该实现网络功能了

## webserver-1.3

    实现网络编程部分，通过调用linux的网络接口系统调用来实现 

    定义Acceptor class，用于accept(2)新TCP连接，并通过回调通知使用者。 

## webserver-1.4

    实现TcpServer TcpConnection 封装Acceptor，更好的管理连接

    实现Buffer，用于数据的输入输出

## webserver-1.5

    实现发送和关闭功能

