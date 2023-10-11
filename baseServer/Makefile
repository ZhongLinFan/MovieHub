TARGET=./bin/baseServer
CXX=g++
CFLAGS=-g -Wall -std=c++11 -fPIC -O2

#依赖的路径
REACTOR=../reactor
PROTOBUF=../tools/protobuf
MYSQLSERVICE=$(PROTOBUF)/mysqlService
LBSERVICE=$(PROTOBUF)/lbService
BASESERVICE=$(PROTOBUF)/baseService
COMMONSERVICE=$(PROTOBUF)/common

#依赖的库 ractor mysql pthread protobuf
REACTORLIB=-L$(REACTOR)/lib -lreactor
MYSQLLIB=$(shell mysql_config --libs)  #这里不指定-L是因为这一条命令包含了-L
PTHREADLIB=-lpthread  #这里不需要指定路径，是因为会在默认路径查找
PROTOBUFLIB=-lprotobuf  #这里不指定-L，是因为也是在默认路径下有的

#动态库
LIB=$(REACTORLIB) $(MYSQLLIB) $(PTHREADLIB) $(PROTOBUFLIB)

#头文件 protobuf和pthread不需要指定头文件路径
#每个头文件路径前面都要加上-I，否则会报找不到对应的头文件
INC=-I./include
INC+=-I$(shell mysql_config --include)
INC+=-I$(REACTOR)/include
#ptorobuf生成的头文件路径
INC+=-I$(MYSQLSERVICE) -I$(LBSERVICE) -I$(COMMONSERVICE) -I$(BASESERVICE)  #注意如果前面一个-I是空的，那么会影响后面的文件查找

#目标文件
#SRC=./src/  错误的，不能用因为下面还有一个SRC/*.cpp
SRC=$(wildcard ./src/*.cpp)
OBJS=$(patsubst %.cpp, %.o, $(SRC))
#protobuf生成的cc文件对应的.o文件
#注意这里需要用到common的.o吗，因为LBSERVCIE的h文件包含了common的头文件，自己的cpp文件也用到了
#看博客发现是需要有的，需要搞明白多文件编译时发生了什么，如果一个文件a用到了另一个文件b的h文件，那么他只会在用到b文件里面定义的变量时，链接阶段 会去对应的文件找到该变量的定义，如果发现没有，就会报错未定义，所以不管是嵌套还是包含，直接给出就行，编译器会自己去找的
PROTOPATH=$(MYSQLSERVICE) $(LBSERVICE) $(COMMONSERVICE) $(BASESERVICE)
PROTOSRC=$(wildcard $(addsuffix /*.cc, $(PROTOPATH)))
OBJS+=$(patsubst %.cc, %.o, $(PROTOSRC))


$(TARGET):$(OBJS)
	mkdir -p bin/
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIB)  #原材料 -o target 或者 -o target 原材料 ，如果有头文件路径需要指定

%.o:%.cpp
	#注意这里一定要指定inc，否则会找不到cpp对应的头文件
	#注意如果不加-o。那么生成的.o文件全都在MakeFile那个路径下
	$(CXX) $(CFLAGS) $< -c -o $@ $(INC)  #这里同样使用上面的原材料位置法则
%.o:%.cc
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)  #这两种写法都对，和上面的-o写法一致，不懂得话看博客

.PHONY:clean

clean:
	rm -f $(TARGET) src/*.o
