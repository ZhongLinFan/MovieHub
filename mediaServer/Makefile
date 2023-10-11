TARGET=./bin/mediaServer
CXX=g++
CXXFLAGS=-g -O2 -Wall -std=c++11

#依赖的库路径，reactor库，mysql库，protobuf库，ffmpeg库，pthread
REACTOR=../reactor
#其他的库都是默认路径，不需要指定
#依赖的其他源文件路径 rtsp protobuf ffmpeg解封装模块
RTSP=../tools/rtsp
RTP_PROTO=$(RTSP)/rtp
RTSP_PROTO=$(RTSP)/rtsp
PROTOBUF=../tools/protobuf
MEDIASERVICE=$(PROTOBUF)/mediaService
MYSQLSERVICE=$(PROTOBUF)/mysqlService
AVDEMUXER=./ffmpeg

#依赖的库
REACTORLIB=-L$(REACTOR)/lib -lreactor
MYSQLLIB=$(shell mysql_config --libs)  #这里不指定-L是因为这一条命令包含了-L
PROTOBUFLIB=-lprotobuf
FFMPEGLIB=-lavformat -lavcodec  #服务器方面只需要这两个库，一个头文件对应一个库，可以看周小哥的那个头文件包含，也是这样的，不需要指定路径，因为这些都是在默认路径下的
PTHREADLIB=-lpthread

#依赖的动态库
LIBS=$(REACTORLIB) $(MYSQLLIB) $(PROTOBUFLIB) $(FFMPEGLIB) $(PTHREADLIB)

#编译需要的头文件
#当前服务器自己的include
#一定不要忘记-I，否则根本找不到
INC=-I./include
#解封装的头文件
INC+=-I$(AVDEMUXER)/include
#rtsp模块
INC+=-I$(RTP_PROTO)/include
INC+=-I$(RTSP_PROTO)/include
#一定不要忘记reactor库的头文件
INC+=-I$(REACTOR)/include
#一定不要忘记ptorobuf的头文件呀，要不然目标文件里有，但是头文件搜不到
INC+=-I$(MEDIASERVICE)
INC+=-I$(MYSQLSERVICE)
#需要的源文件
CPPPATH=./src $(AVDEMUXER)/src $(RTP_PROTO)/src $(RTSP_PROTO)/src
CPPSRC=$(wildcard $(addsuffix /*.cpp, $(CPPPATH)))
CPPOBJS=$(patsubst %.cpp, %.o, $(CPPSRC))


#因为protobuf是cc文件所以需要单独添加
CCPATH=$(MEDIASERVICE) $(MYSQLSERVICE)
CCSRC=$(wildcard $(addsuffix /*.cc, $(CCPATH)))
CCOBJS=$(patsubst %.cc, %.o, $(CCSRC))

OBJS= $(CPPOBJS) $(CCOBJS)

$(TARGET):$(OBJS)
	mkdir -p ./bin
	echo $(CPPOBJS)
	echo $(CPPPATH)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIBS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) #需要指定头文件，动态库不确定要不要指定
%.o:%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INC)

.PHONY:clean clear

clean:
	rm -rf ./src/*.o
clear:
	rm -rf $(CPPOBJS)





