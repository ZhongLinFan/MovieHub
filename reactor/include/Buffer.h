#pragma once
#include "Log.h"
#include "BufferPool.h" // 这样可以使用BufferPool类型、MScale、MUnit类型的数据

//后来发现名字太长了，有些没必要
#include <pthread.h>  //再写客户端的时候发现需要在这里加锁，所以这里操作一下，然后想尝试c版本的互斥锁和c++版本的，所以这里使用c版本的

//先声明，否则用不了，真用不了，即使包含头文件
struct MUnit;
class Buffer {
public:
	Buffer();
	//不管是udp还是tcp的客户端都使用不了那么大的内存池，所以按照普通的方法开辟就行了
	//这样既可以通过内存池的方式，又可以通过new的方式开辟，会让整个代码不是那么清晰，所以这个混合在一起的不知道是不是不好
	//可以采取一个更好的方式，就是一个父类，两个子类的方式
	//先这样吧。。。
	Buffer(int size);
	void auxiliaryInit(); //辅助初始化函数
	//销毁一个Buffer
	~Buffer();
	//扩容Buffer,尝试性扩容，如果需要的size能够满足写入的大小，就不扩容
	bool resizeBuffer(int size);
	//获取可写区域大小(最后面的空闲区域)
	int getWriteSize();
	//获取可（未）读区域大小 // 实际数据长度
	int getReadSize();
	//向buffer写入数据
	int writeToBuffer(const char* data, int size);
	int writeToBuffer(Buffer* buf, int size);
	//在写Message的解析时感觉非常需要这么一个函数，就是从当前位置读到另一个位置dst
	//读多少字节,如果度读的字节数过大，那么读getReadSize()这么大的字节，返回读到的字节数
	// 注意为啥没有bool Buffer::readFromBuffer(Buffer* buf, int size)，因为和writeToBuffer一模一样
	//dst可以指定为nullptr
	int readFromBuffer(char* dst, int size, bool copy);
	//读到以某个字符或者字符串截至为止，如果没有那么返回空，如果有返回截止字符串的位置的前一个位置比如abcd，cd，那么返回b的位置
	//注意这个str可能会导致内存泄露，str如果是new出来的，那么就会泄露，应该采取const char* data  = "abcd"， char* str = data;
	//由于可能会泄露，所以直接不读了,而是采用下面的方式，返回读到的字节数，
	int readFromBuffer(char* dst, const char* str);

	//还要有一个读Buf，用来帮助外界指针指向对应的数据起始地址,但是发现上面增加一个参数比较好
	//bool readFromBuffer(char* dst, int size);
	//还要有一个char** dst类型的参数，因为我只想指向当前数据，不想拷贝
	int readFromBuffer(char** dst, int size);
protected:
	int copyData(const char* start, int size);//上面两个的辅助函数
	bool swapMUnit(int size); //换一个更大的内存
	//为了保证扩容安全，必须要给读指针一个函数，然后其他成员函数访问readPos必须从这里获得
	//int getReadPos() {
		//while (!isResized) {
			//pthread_cond_wait(&m_resizeMemCond, m_resizeMemMutex);
		//}
		//return m_readPos;
	//}
public:
	char* m_data;
	int m_readPos;
	int m_writePos;
	int m_capacity;
	pthread_mutex_t m_rMutex;  //读写buf都有自己的互斥锁，不过更好的情况是读写buf都有自己的读写互斥锁，那就改成这样吧，但其实读buf的写操作是在这里面，
								//但是读操作是在msg那里，同样，写buf的读是在这里面，但是写是在sendMsg那里，但是他们调用的都是这里的接口，所以这里还是有必要提供读写锁的
								//其实这里有一个微小的漏洞，就是读写的时候需要扩容，这个扩容不能对其进行加锁和解锁，因为在读写的时候已经加了锁，然后你扩容在加锁，直接阻塞了。
								//但是如果扩容不加锁，会导致一个进程在读，一个进程在写，写的时候扩容，又导致移动数据或者换内存，就会出现在去读的时候出现问题，这时候应该为扩容单独增加一个锁
								//这样就可以避免上述问题，比如一个线程在写，写的时候扩容，这个时候读线程不需要扩容，这好像也会出现问题吧，确实有问题，比如扩容还没来得及更新m_readPos，这个时候
								//读线程去读肯定会出问题的，这该咋办，使用条件变量，需要认识到只有写到buf的时才需要扩容，这个扩容只会对读操作有影响，因为其他的写进程是互斥的，读进程只会操作读指针，
								// 在获取，所以可以单独提供一个获取读指针的函数，在这个函数里，在获取读指针的时候需要等待一下扩容之后的条件变量，这样就可以完美解决这个读写安全问题了
								//比如一个写进程在写的过程中，另一个读进程也开始读了，写进程写的时候需要扩容，这个时候读进程刚好要获取读指针，这个时候写进程的扩容如果没有结束，那么这个
								//读进程是要阻塞的，（如果不阻塞，就会出现获取到旧的地址（相对地址），但是旧的地址加上新的m_data就会出现问题了），
								//这个时候如果阻塞完成，那么读进程获取读指针就是正确的，但是需要保证读进程内不能使用变量保留旧的读地址，反正只要存储获取读地址，都要进行条件等待
								//那应该在父类中提供一个函数获取读指针的函数
	pthread_mutex_t m_wMutex;  
	//pthread_cond_t m_resizeMemCond;
	//pthread_mutex_t m_resizeMemMutex; //条件变量需要搭配一个互斥锁使用（可以使用读锁吗，万一其他读线程已经在进行读操作呢，这个时候其他线程已经对读锁加锁了，
										//你这个写线程再在扩容里使用读锁加锁，会导致当前写进程等待，等到读完成，再加锁（防止后续读线程再读），但是这个条件变量已经没必要了，这样其实也可以，
										//其实上面这种方法是可以的，就是resize的时候，直接加读锁，如果加读锁失败，那肯定是其他线程在使用读锁，
										//因为当前线程只有写的时候才会扩容，所以当前线程加的锁肯定都是写锁，我看了一下发现确实resize之前都是写锁，那么我resize的时候，就先加读锁，如果等待
										//那就等其他线程读完，如果不等待，那就没事了，那我直接加读锁，所以这个逻辑已经很清晰了，当然使用条件变量肯定也可以，就是当前属性设置一个变量表达
										//是否扩容完成，然后getReadPos一直检查是否扩容完成，如果完成就直接返回pos，如果没有完成，那肯定要等待扩容中的条件变量通知接触阻塞
										//难道不能直接while等待为true吗，不也一样？扩容完成直接设为true不就行了？但是当你while的时候，其他线程肯定休眠了呀，
										//而其他线程解除阻塞了，已经完工了，你肯定要休息了，所以需要条件变量提醒一下，所以三种方法都可以，
										//一个是直接加读锁，另外一种是条件变量（独立的锁），还有一种是啥都不用，wait等待）我选择使用条件变量，可能直接加读锁更优雅一些吧,但是我想试验一下
										//算了还是直接加读锁吧，因为如果使用条件变量，再设置个bool值，很奇怪的感觉
	//bool isResized;
private:
	MUnit* m_unit; //为了内存池的设计，所以不希望子类继承该属性，只是在构造函数里传递m_data和m_capacity
};

class ReadBuffer :public Buffer {
public:
	ReadBuffer();
	ReadBuffer(int size);
	~ReadBuffer();
	//buffer接收套接字数据，读fd的数据到buf
	int readSocketToBuffer(int fd);
	//上面是针对Tcp的，下面是Udp的
	int readSocketToBuffer(int fd, sockaddr_in* sockAddr, socklen_t* addrlen);
};

class WriteBuffer :public Buffer {
public:
	WriteBuffer();
	WriteBuffer(int size);
	~WriteBuffer();
	//考虑一个写缓冲区写到fd，之前设计的不合理，因为有读肯定在这里要有写，而我之前的写是在cfdconnect的函数里不是以函数的形式发送出去的，显然不合理
	int writeBufferToSocket(int fd);  //但是因为底层发送数据方式不同，这里先简单实现一个响应消息全都在buf里的
	// 上面是针对Tcp的，下面是Udp的
	int writeBufferToSocket(int fd, int pkgLen, sockaddr_in* sockAddr, socklen_t addrlen);
};

