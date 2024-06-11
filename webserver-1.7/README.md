# webserver-1.7

实现主动建立连接

实现Connector类，负责建立socket连接

TcpClient类，封装了客户端操作，调用Connector接口，建立连接，收发数据，和TcpServer不同的是每个TcpClient只管理一个TcpConnection。