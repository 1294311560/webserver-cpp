main : main.o Poller.o Channel.o EventLoop.o 
	g++ main.o Channel.o EventLoop.o Poller.o -o main
main.o : main.cpp Channel.o EventLoop.o Poller.o
	g++ -g -c main.cpp
Poller.o : Poller.cpp
	g++ -g -c Poller.cpp
Channel.o : Channel.cpp
	g++ -g -c Channel.cpp
EventLoop.o : EventLoop.cpp
	g++ -g -c EventLoop.cpp

clear:
	rm -r *.o main