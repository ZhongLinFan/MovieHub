CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

INC=-I./../../include
LIB=-L./../../lib -lreactor -lpthread
OBJS=$(patsubst %.cpp, %.o, $(wildcard *.cc)) #这个*.cc没搞懂干啥

all:
	$(CXX) server.cpp -o server $(CFLAGS) $(INC) $(LIB)
	$(CXX) client.cpp -o client $(CFLAGS) $(INC) $(LIB)

.PHONY:clean

clean:
	-rm -f *.o reactor
