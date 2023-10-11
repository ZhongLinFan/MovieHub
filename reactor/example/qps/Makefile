CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

INC=-I./../../include
LIB=-L./../../lib -lreactor  -lprotobuf #博主把lpthread放在这了，我认为不好，我感觉更应该放到reactor那里
#OBJS=$(patsubst %.cc, %.o, $(wildcard *.cc)) #这个*.cc没搞懂干啥

all:
	$(CXX) msg.pb.cc server.cpp -o server $(CFLAGS) $(INC) $(LIB)
	$(CXX) msg.pb.cc client.cpp -o client $(CFLAGS) $(INC) $(LIB)

.PHONY:clean

clean:
	-rm -f *.o reactor
