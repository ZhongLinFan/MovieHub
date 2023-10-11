TARGET=bin/mysqlAgent
CXX=g++
CFLAGS= -g -O2 -Wall -fPIC -std=c++11 #不明白为啥是CFLAGS -fPIC是生成库的选项
#不应该有-fPic了,但是因为已经打包了，就没有改

SRC=./src
INC=-I./include $(shell mysql_config --include) #这个是根据chatgpt知道的指令已经在网上搜索怎么使用shell命令嵌套的,注意不需要加-I，关于mysql的路径
OBJS=$(addsuffix .o, $(basename $(wildcard $(SRC)/*.cpp)))
#或者OBJS=$(patsubst %.cpp, %.o $(SRC/*.cpp))

LIB=$(shell mysql_config --libs) -lpthread #博主把lpthread房到了example那里了，不过也是可以的，因为在网上看到https://www.zhihu.com/question/36236334，如果不加只会在运行的时候报错，所以在这里加，或者是在上面加都是可以的，为了保证哪里用哪里加，我就在这加了，其实mysql里已经加了, 博主的意思差不多也是这个意思，就是这个reactor依赖pthread，但是他是选择在上层添加

#reactor模块,一定要把-I带上
INC+=-I../reactor/include
#LIB+=-I../reactor/lib  这样写是错的，会找i不到实现，而且不是-I而是-L
LIB+=-L../reactor/lib -lreactor

#把protobuf的头文件、源文件和动态库加上
PROTO_MYSQL_SERVICE=../tools/protobuf/mysqlService
PROTO_COMMON_USER=../tools/protobuf/common
INC+=-I$(PROTO_MYSQL_SERVICE)
INC+=-I$(PROTO_COMMON_USER) #注意，因为上面的proto使用了共享的proto文件，本质上是包含了他的头文件，所以需要把共享的目录包含在这里
#注意这里只给出了.o文件，那么势必要执行%.o: %.cpp，但是make可以自动推导，即使没有合适的cpp文件，也会找到合适的依赖文件所以这里只需要指定.o文件即可，也就是找不到router.pb.cpp的时候，会自动找到router.pb.cc文件然后编译成.o
OBJS+=$(PROTO_MYSQL_SERVICE)/mysqlService.pb.o
OBJS+=$(PROTO_COMMON_USER)/user.pb.o   #必须要包含这个，如果不包含这个会在连接阶段报找不到定义
LIB+=-lprotobuf

$(TARGET):$(OBJS)
	mkdir -p bin
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIB)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o  $@ $< $(INC) 

%.o: %.cc

	$(CXX) $(CFLAGS) -c -o  $@ $< $(INC) 

.PHONY: clean

clean:
	rm -f src/*.o $(TARGET) $(PROTO_MYSQL_SERVICE/*.o)
