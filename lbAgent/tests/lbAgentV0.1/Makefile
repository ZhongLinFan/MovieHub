TARGET=./client
CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11

#依赖的模块基路径
REACTOR=../../../reactor
PROTOBUF=../../../tools/protobuf

#reactor模块
REACTOR_LIB=-L$(REACTOR)/lib -lreactor
REACTOR_INC=-I$(REACTOR)/include

#protobuf模块
PROTOBUF_LIB=-lprotobuf
PROTOBUF_INC=-I$(PROTOBUF)/lbService

#当前模块
#SRC=./src/  错误的，不能用因为下面还有一个SRC/*.cpp
SRC=$(wildcard ./*.cpp)

#加上上面两个模块的东西
INC=$(REACTOR_INC) $(PROTOBUF_INC)
LIB=$(REACTOR_LIB) $(PROTOBUF_LIB) -lpthread   #reactor用到了动态库，这里也要加, 否则会报错
#最终源文件需要当前模块的和probuf的源文件和头文件，一起编译，所以需要下面这一句
OBJS=$(PROTOBUF)/lbService/lbService.pb.o

$(TARGET):$(OBJS)
	#$(CXX) $(CFLAGS) -o $(OBJS) $(INC) $(LIB) 这是不对的，这样就会以$(OBJS)为目标了
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRC) $(OBJS) $(INC) $(LIB)  #注意，当前文件夹下不需要.o

%.o:%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)
%.o:%.cc
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)

.PHONY: clean

clean:
	-rm -rf $(TARGET) $(OBJS)

