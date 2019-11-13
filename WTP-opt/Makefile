CC=gcc
CXX=g++
RM=rm -f
LDFLAGS = -g -std=c++11
TP=../starter_files/
TOOLS=$(wildcard $(TP)*.h)

all: send recv

send: wSender.o session_send.o
	$(CXX) $(LDFLAGS) -o wSender wSender.o session_send.o

wSender.o: wSender.cpp session_send.h
	$(CXX) $(LDFLAGS) -c wSender.cpp

session_send.o: session_send.h $(TOOLS)
	$(CXX) $(LDFLAGS) -c -I $(TP) session_send.cpp

recv : wReceiver.o utils.o session_receive.o SlidingWindow.o
	g++ -std=c++11 -o wReceiver wReceiver.o utils.o session_receive.o SlidingWindow.o

wReceiver.o : wReceiver.cpp session_receive.h
	g++ -std=c++11 -c wReceiver.cpp

session_receive.o : session_receive.cpp session_receive.h SlidingWindow.h utils.h
	g++ -std=c++11 -c session_receive.cpp

SlidingWindow.o : SlidingWindow.cpp SlidingWindow.h
	g++ -std=c++11 -c SlidingWindow.cpp

utils.o : utils.cpp
	g++ -std=c++11 -c utils.cpp

clean:
	$(RM) *.o wSender wReceiver
