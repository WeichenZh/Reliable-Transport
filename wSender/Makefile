CC=gcc
CXX=g++
RM=rm -f
LDFLAGS = -g -std=c++11
TP=../starter_files/
TOOLS=$(wildcard $(TP)*.h)

all: send rpt

rpt: repeater.o
	$(CXX) $(LDFLAGS) -o repeater repeater.o

repeater.o: repeater.cpp
	$(CXX) $(LDFLAGS) -c repeater.cpp

send: wSender.o session_send.o
	$(CXX) $(LDFLAGS) -o wSender wSender.o session_send.o

wSender.o: wSender.cpp session_send.h
	$(CXX) $(LDFLAGS) -c wSender.cpp

session_send.o: session_send.h $(TOOLS)
	$(CXX) $(LDFLAGS) -c -I $(TP) session_send.cpp


clean:
	$(RM) *.o wSender wReceiver
