CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

INC=-I./../include
LIB=-L./../lib -lreactor -lpthread
OBJS=$(patsubst %.cpp, %.o, $(wildcard *.cc)) #这个*.cc没搞懂干啥

all:
	$(CXX) main.cpp -o reactor $(CFLAGS) $(INC) $(LIB)

.PHONY:clean

clean:
	-rm -f *.o reactor
