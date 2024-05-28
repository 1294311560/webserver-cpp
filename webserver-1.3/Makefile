SRC_FILES=main.cpp Poller.cpp Channel.cpp EventLoop.cpp Timestamp.cpp Timer.cpp TimerQueue.cpp EventLoopThread.cpp SocketsOp.cpp Socket.cpp InetAddress.cpp Acceptor.cpp
OBJ_FILES=$(SRC_FILES:.cpp=.o)

main : $(OBJ_FILES) test
	g++ $(OBJ_FILES) -o main

%.o : %.cpp
	g++ -c -o $@ $<
test: test.cpp
	g++ test.cpp -o test
clear:
	rm -r *.o main