# webserver-1.3

实现网络编成部分，通过调用linux的网络接口系统调用来实现

SocketsOp 封装了一些socket的操作，包括创建socket、close、listen、bind、accept，以及字序转化

Socket封装了socket文件描述符，并通过RAII来管理该socket，当socket生命周期结束时，会通过析构函数自动调用close关闭socket

定义Acceptor class，用于accept(2)新TCP连接，并通过回调通知使用者。  
Acceptor的socket是一个listening socket，即server socket。acceptChannel_l用于观察此socket上的readable事件，并回调Acceptor::handleRead()

## 流程

Acceptor会创建一个Socket： acceptSocket_，和一个Channel：acceptChannel_。 

使socket进入listen状态

acceptChannel_会通过loop监听socket，当有连接请求到来时，会调用回调函数Acceptor::handleRead()，该函数会调用accept来接受连接


[linux 网络编程](https://blog.csdn.net/qq_39855356/article/details/126851234)
