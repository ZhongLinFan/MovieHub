TARGET=./bin/lbServer
CXX=g++
CFLAGS=-g -Wall -O2 -std=c++11

#依赖的模块基路径
REACTOR=../reactor
PROTOBUF=../tools/protobuf
PROTO_MYSQL_SERVICE=$(PROTOBUF)/mysqlService
PROTO_LB_SERVICE=$(PROTOBUF)/lbService
PROTO_COMMON_USER=$(PROTOBUF)/common
PROTO_MEDIA_SERVICE=$(PROTOBUF)/mediaService
#reactor模块
REACTOR_LIB=-L$(REACTOR)/lib -lreactor
REACTOR_INC=-I$(REACTOR)/include

#protobuf模块
PROTOBUF_LIB=-lprotobuf
PROTOBUF_INC=-I$(PROTO_MYSQL_SERVICE) -I$(PROTO_LB_SERVICE) -I$(PROTO_COMMON_USER) -I$(PROTO_MEDIA_SERVICE)

#当前模块
#SRC=./src/  错误的，不能用因为下面还有一个SRC/*.cpp
SRC=$(wildcard ./src/*.cpp)
INC=-I./include
OBJS=$(patsubst %.cpp, %.o, $(SRC))

#加上上面两个模块的东西
INC+=$(REACTOR_INC) $(PROTOBUF_INC)
LIB=$(REACTOR_LIB) $(PROTOBUF_LIB) -lpthread   #reactor用到了动态库，这里也要加, 否则会报错
#最终源文件需要当前模块的和probuf的源文件和头文件，一起编译，所以需要下面这一句
OBJS+=$(PROTO_MYSQL_SERVICE)/mysqlService.pb.o
OBJS+=$(PROTO_LB_SERVICE)/lbService.pb.o
OBJS+=$(PROTO_COMMON_USER)/user.pb.o
OBJS+=$(PROTO_MEDIA_SERVICE)/mediaService.pb.o

$(TARGET):$(OBJS)
	mkdir -p bin #这个不要少
	#$(CXX) $(CFLAGS) -o $(OBJS) $(INC) $(LIB) 这是不对的，这样就会以$(OBJS)为目标了
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIB)

%.o:%.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)
%.o:%.cc
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)

.PHONY: clean

clean:
	-rm -rf $(TARGET) $(OBJS)

