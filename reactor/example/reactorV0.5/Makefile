CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

INC=-I./../../include
LIB=-L./../../lib -lreactor -lpthread -lprotobuf
#OBJS=$(patsubst %.cc, %.o, $(wildcard *.cc)) #这个*.cc没搞懂干啥

all:
	$(CXX) msg.pb.cc server.cpp -o server $(CFLAGS) $(INC) $(LIB)
	$(CXX) msg.pb.cc client.cpp -o client $(CFLAGS) $(INC) $(LIB)

.PHONY:clean

clean:
	-rm -f *.o server client
