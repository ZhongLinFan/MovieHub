TARGET=./client
CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11 -fPIC

#依赖的路径
REACTOR=../../../reactor
PROTOBUF=../../../tools/protobuf
BASESERVICE=$(PROTOBUF)/baseService
LBSERVICE=$(PROTOBUF)/lbService
COMMON=$(PROTOBUF)/common  #虽然自己的文件没用到但是上面的文件用到了，如果不加会报错
#依赖的库
LIB=-L$(REACTOR)/lib -lreactor -lpthread -lprotobuf

#依赖的头文件
INC=-I$(REACTOR)/include -I$(BASESERVICE) -I$(LBSERVICE) -I$(COMMON)

SRC=$(wildcard  ./*.cpp)
OBJS=$(patsubst %.cpp, %.o, $(SRC))
PROTOPATH=$(BASESERVICE) $(LBSERVICE) $(COMMON)
PROTOSRC=$(wildcard $(addsuffix /*.cc, $(PROTOPATH)))
OBJS+=$(patsubst %.cc, %.o,$(PROTOSRC))

$(TARGET):$(OBJS)
	echo $(SRC)
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIB)

%.o:%.cc
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)
%.o:%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)
.PHONY:clean

clean:
	-rm -f $(TARGET) 
