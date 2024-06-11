SRC_FILES=main.cpp Poller.cpp Channel.cpp EventLoop.cpp Timestamp.cpp Timer.cpp TimerQueue.cpp EventLoopThread.cpp \
 SocketsOp.cpp Socket.cpp InetAddress.cpp Acceptor.cpp TcpConnection.cpp TcpServer.cpp Buffer.cpp EventLoopThreadPool.cpp \
 Connector.cpp TcpClient.cpp
OBJ_FILES=$(SRC_FILES:.cpp=.o)

main : $(OBJ_FILES) 
	g++ $(OBJ_FILES) -o main

%.o : %.cpp
	g++ -c -o $@ $<
clear:
	rm -r *.o main
