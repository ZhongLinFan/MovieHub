TARGET=lib/libreactor.a
CXX=g++
CFLAGS= -g -O2 -Wall -fPIC -std=c++11 #不明白为啥是CFLAGS -fPIC是生成库的选项


SRC=./src
INC=-I./include 
OBJS=$(addsuffix .o, $(basename $(wildcard $(SRC)/*.cpp)))
#或者OBJS=$(patsubst %.cpp, %.o $(SRC/*.cpp))

LIB=-lpthread #博主把lpthread房到了example那里了，不过也是可以的，因为在网上看到https://www.zhihu.com/question/36236334，如果不加只会在运行的时候报错，所以在这里加，或者是在上面加都是可以的，为了保证哪里用哪里加，我就在这加了，其实mysql里已经加了, 博主的意思差不多也是这个意思，就是这个reactor依赖pthread，但是他是选择在上层添


$(TARGET):$(OBJS)
	mkdir -p lib
	ar cqs $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o  $@ $< $(INC) $(LIB)

.PHONY: clean

clean:
	rm -f src/*.o $(TARGET)
