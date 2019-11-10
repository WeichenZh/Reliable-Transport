wReceiver : wReceiver.o utils.o session_receive.o SlidingWindow.o
	g++ -o wReceiver wReceiver.o utils.o session_receive.o SlidingWindow.o

wReceiver.o : wReceiver.cpp session_receive.h
	g++ -c wReceiver.cpp

session_receive.o : session_receive.cpp session_receive.h SlidingWindow.h utils.h
	g++ -c session_receive.cpp

SlidingWindow.o : SlidingWindow.cpp SlidingWindow.h
	g++ -c SlidingWindow.cpp

utils.o : utils.cpp
	g++ -c utils.cpp

clean :
	rm -f wReceiver wReceiver.o SlidingWindow.o session_receive.o utils.o
