TARGET=server
CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

REACTOR=./../../../reactor
PROTOBUF=./../../../tools/protobuf/mysqlService

INC=-I$(REACTOR)/include -I$(PROTOBUF)
LIB=-L$(REACTOR)/lib -lreactor -lpthread -lprotobuf
OBJS=$(PROTOBUF)/mysqlService.pb.o  #一定要把cc文件一起编译，因为如果不一起编译，只是加了动态库和头文件，会发现任何函数都使用不了

$(TARGET):$(OBJS)
	$(CXX) server.cpp -o server $(CFLAGS) $(OBJS) $(INC) $(LIB)

%.o:%.cc
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)

.PHONY:clean

clean:
	-rm -f *.o $(TARGET)
