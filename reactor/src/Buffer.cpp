#include "Buffer.h"
#include <string.h> //memcpy等 ，没有memncpy这个函数
#include <errno.h>
#include <assert.h> //assert函数
#include <stdlib.h> // realloc
#include <unistd.h>//read write
#include "Message.h" // 需要用到里面的宏

Buffer::Buffer()
{
	//这个是供给内存池开辟空间用的，子类如果用继承构造，那线程池就没意义了
	m_unit = BufferPool::getInstance()->allocm();//因为加了#include "BufferPool.h"所以可以访问BufferPool类里面的静态成员
	m_data = m_unit->m_data;
	m_capacity = m_unit->m_capacity;
	auxiliaryInit();
}

Buffer::Buffer(int size)
{
	m_unit = nullptr;  //客户端用不到
	m_data = new char[size];
	m_capacity = size;
	auxiliaryInit();
}

void Buffer::auxiliaryInit() {
	assert(m_data);
	m_readPos = 0;
	m_writePos = 0;
	pthread_mutex_init(&m_rMutex, NULL);  //c的还是指定为NULL
	pthread_mutex_init(&m_wMutex, NULL);
	Debug("获得一块内存，当前内存块的大小为：%d", m_capacity);
}

Buffer::~Buffer()
{
	//这里应该是当前连接已经释放了，因为buf都不用了，那么TCPconnnection肯定也不需要了，因为他俩是一起的;
	//所以可以在这里将所有的资源归还
	if (m_unit != nullptr) {
		BufferPool::getInstance()->recycle(m_unit);
	}
	else {
		delete m_data;
	}
	m_data = nullptr;
}

bool Buffer::resizeBuffer(int size)
{
	bool isResizedBuffer = true;

	Debug("resizeBuffer加锁");
	if (size < getWriteSize()) {
		Debug("当前大小：%d,需要扩容的大小：%d", getWriteSize(), size);
	}
	
	//移动未读区域之后才可以写的下
	else if (size < m_readPos + getWriteSize()) {
		//将未读区域移动到开头的位置
		Debug("扩充Buf大小，将未读区域移动到开头的位置");
		pthread_mutex_lock(&m_rMutex);  //加读锁
		int readable = getReadSize();
		//这里不用在判断m_readPos和readable是否为0的情况，因为前面已经把这些情况考虑过了
		memcpy(m_data, m_data + m_readPos, readable); // memmove是从后往前拷贝
		m_readPos = 0;
		m_writePos = readable;
		pthread_mutex_unlock(&m_rMutex);  //解读锁
	}
	else {
		//如果m_unit是为空，那么可以判断是客户端，那么就正常开辟
		if (m_unit == nullptr) {
			//采用下面的两倍扩充
			Debug("扩充Buf大小为两倍");
			pthread_mutex_lock(&m_rMutex);  //加读锁
			m_data = (char*)realloc(m_data, m_capacity + size * 2);
			assert(m_data != nullptr);
			//memset(buf->capacity, 0, size);  不是buf->capacity作为地址呀，在操作类似情况时，要时刻记得相对地址和绝对地址
			memset(m_data + m_capacity, 0, size * 2);
			m_capacity += size * 2;
			pthread_mutex_unlock(&m_rMutex);  //加读锁
			Debug("Buf的大小为：%d", m_capacity);
		}
		else {
			//如果内存不够那么就换一个更大的
			//没有换成功，说明当前申请的内存太大了，或者所有比当前内存块大的都被申请完了，并且已经达到总内存已经达到上限值
			//那么就等一会，等到消费者把已写的消费完，或者等其他进程归还,现在直接return false TODO
			pthread_mutex_lock(&m_rMutex);  //加读锁
			if (!swapMUnit(size)) {
				isResizedBuffer = false;
			}
			pthread_mutex_unlock(&m_rMutex);  //加读锁
			Debug("更换了更大的内存块：当前内存块的大小为：%d", m_capacity);
		}
	}
	Debug("resizeBuffer解锁");
	
	return isResizedBuffer;  //其他分支都是return true
}

int Buffer::getWriteSize()
{
	return m_capacity - m_writePos;
}

int Buffer::getReadSize()
{
	return m_writePos - m_readPos;
}

int Buffer::writeToBuffer(const char* data, int size)
{
	//注意这里由于可以扩充，所以可以不判断数据的大小是否很大
	//返回-1代表参数有问题
	//返回0 代表实际写入的字节（这个是正常的，因为很多时候查的没有这个东西，需要返回0）
	//返回大于0，代表实际写入的字节	
	pthread_mutex_lock(&m_wMutex);//写锁加锁
	Debug("writeToBuffer1加锁");
	int writed = copyData(data, size);
	if (writed >= 0) {
		m_writePos += writed;
		Debug("writeToBuffer1解锁");
		pthread_mutex_unlock(&m_wMutex); //写锁解锁 注意每个return前面都要解锁
		return writed;
	}
	Debug("writeToBuffer1解锁");
	pthread_mutex_unlock(&m_wMutex);//写锁解锁
	return writed;
}

int Buffer::writeToBuffer(Buffer* buf, int size)
{
	//但是这里的buf就应该判断一下，以防止越界
	pthread_mutex_lock(&m_wMutex);//写锁加锁
	Debug("writeToBuffer2加锁");
	if (size > buf->getReadSize()) {
		size = buf->getReadSize();
	}
	if (copyData(buf->m_data + buf->m_readPos, size) > 0) {
		m_writePos += size;
		buf->m_readPos += size;
	}
	Debug("writeToBuffer2解锁");
	pthread_mutex_unlock(&m_wMutex);  //写锁解锁
}

//辅助上面的两个函数，但是不能辅助read函数
int Buffer::copyData(const char* start, int size)
{
	//在写mysql的时候发现，结果为0还是很常见的，不能直接就不发了
	if (start == nullptr  || size < 0) {
		return -1;
	}
	if (size == 0) {
		return 0;
	}
	resizeBuffer(size);
	memcpy(m_data + m_writePos, start, size);
	return size;
}

int Buffer::readFromBuffer(char* dst, int size, bool copy)
{
	Debug("readFromBuffer正在执行，size：%d, getReadSize:%d", size, getReadSize());
	if (size <= 0) {
		return 0;
	}
	//if (size > getWriteSize()) {
	//	size = getWriteSize();
	//}这是错误的，不应该判断写空间，而应该判断读空间
	pthread_mutex_lock(&m_rMutex); //读锁加锁
	Debug("readFromBuffer1加锁");
	if (size > getReadSize()) {
		size = getReadSize();
	}
	if (dst == nullptr && !copy) {
		m_readPos += size;
		Debug("readFromBuffer1解锁");
		pthread_mutex_unlock(&m_rMutex); //注意每个return后面都应该分别解锁。。。。这个可以当作很好的bug去预防。。。。。
		return size;
	}
	if (dst && copy) {
		//m_readPos += size;   //一定要加这句话呀！！！！！！,我日你大爷！！！！！！！！！！！你为啥要加在这。。。。。
		//排查了一下午了了呀！！！！！！！！还没拷贝就加，是不是拷贝的地址就不对了，应该拷贝完再加呀！！！！
		memcpy(dst, m_data + m_readPos, size);
		m_readPos += size;
		Debug("readFromBuffer1解锁");
		pthread_mutex_unlock(&m_rMutex);
		return size;
	}
	Debug("readFromBuffer1解锁");
	pthread_mutex_unlock(&m_rMutex);  //这里也要去解锁，万一上面的都没进去，那就需要在这里解锁
	return 0;
#if 0
	//这是错误的，因为*dst 需要char**类型的形参
	else {
		//dst = m_readPos; 错误的，相对地址
		*dst = m_data + m_readPos; 
	}
#endif
	m_readPos += size;
	return size;
}

int Buffer::readFromBuffer(char* dst, const char* str)
{
	//memmem
	//这里有一个隐患，我的m_data回收时是没清空的，所以如果匹配到垃圾数据怎么办
	//可以增加一个结束符号，但是。。。可以在下面简单做一个判断
	//找到了，看看是否在有效范围内，有效，那么返回
	pthread_mutex_lock(&m_rMutex); //读锁加锁
	Debug("readFromBuffer2加锁");
	char* end = strstr(m_data + m_readPos, str);
	if (end == nullptr || end >= m_data + m_writePos) {
		return -1;  //说明不存在
	}
	//拷贝
	//int size = end - m_readPos;实际地址和相对地址比较
	int size = end - (m_data + m_readPos);
	memcpy(dst, m_data + m_readPos, size);
	//更新buf信息
	m_readPos = end - m_data + strlen(str);  //str("123")为3
	assert(m_readPos <= m_writePos);
	Debug("readFromBuffer2解锁");
	pthread_mutex_unlock(&m_rMutex); //读锁解锁
	return size; //可能为0

}

int Buffer::readFromBuffer(char** dst, int size)
{
	pthread_mutex_lock(&m_rMutex); //读锁加锁
	Debug("readFromBuffer3加锁");
	if (size > getReadSize()) {
		size = getReadSize();
	}
	if (dst) {
		*dst = m_data + m_readPos;
		m_readPos += size;  //不加的话读满一次之后，直接出错
		Debug("readFromBuffer3解锁");
		pthread_mutex_unlock(&m_rMutex);  //读锁解锁
		return size;
	}
	Debug("readFromBuffer3解锁");
	pthread_mutex_unlock(&m_rMutex);  //读锁解锁，一定要在外面再解开一次，因为如果是第一个if，第二个if没进去，就一直加锁了
	return 0;	
}

bool Buffer::swapMUnit(int size)
{
	//注意这里即使是ReadBuffer调用resizeBuffer，尽管swapMUnit是私有的，那么也是可以被执行的，因为继承了resizeBuffer这个函数
	//这是在类内部执行的，不是外面，所以这里也可以访问私有变量
	//初衷是换一个更大的内存块，但是如果相等或者跟小就划不来了，当然后期如果有需要可以考虑换成小的
	if (size <= m_capacity) {
		Debug("为什么要换成小内存呢");
		Debug("当前需要读的数据为：%d，当前可写空间为：%d，当前读指针：%d", size, getWriteSize(), m_readPos);
		return true;
	}
	MUnit* old = m_unit;
	m_unit = BufferPool::getInstance()->allocm(size);
	if (m_unit != nullptr) {
		m_capacity = m_unit->m_capacity;
		//由于可能在读fd里面的值的时候，需要不断扩容，这个时候在刚刚以前读到的信息不能丢，所以这里应该提供拷贝原来   未读信息   的功能
		//由于有读写指针，所以，在回收的时候不用清空，因为我需要的数据都被读写指针限制了
		int unread = getReadSize();
		memcpy(m_unit->m_data, m_data + m_readPos, unread);
		//这个一定一定要记得更新！！！！！突然灵光一线才想起来
		m_readPos = 0;
		m_writePos = unread;
		//BufferPool::getInstance()->recycle(m_unit);，这个不能这样释放呀，刚申请就释放。。。。
		BufferPool::getInstance()->recycle(old);
		return true;
	}
	m_unit = old; // 所以要换回来呀
	return false; // 执行失败不能破坏原来的内存呀
}

//======ReadBuffer=======
ReadBuffer::ReadBuffer():Buffer(){
	//这里什么都不需要做，直接返回一个从内存池开辟好的内存块
	//但是构造函数又不能return,这个时候发现了问题的弊端，其实开辟内存时我只需要内存空间加一个next指针，而不是在BufferPool构造函数中
	// 开辟内存空间时，进行之前的Buffer(int)
	//其他的参数我根本不需要，也就是如果当前Buffer的指针如果数据块不够大，你给我换一块就行了，
	//当前结构体只有一个指针，除了构造函数，其他的函数均没有
	// 父类则有一个结构体的属性，子类可以继承，当子类初始化时，可以通过父类的构造函数得到一个结构体实例，不管父类还应该增加
	// 归还和交换机制，注意这个结构体应该属于谁呢，应该属于BufferPool，Buffer只关心有效的内存
	//所以综上所述，如果只有Buffer和读写Buffer根本行不通，必须再增加一个结构体
	///不对，归还的时候需要大小才能归还到正确位置，所以需要next和capacity

	//改完之后，可以直接继承父类的构造函数，其他的什么都不用管
}

ReadBuffer::ReadBuffer(int size):Buffer(size)
{
}

ReadBuffer::~ReadBuffer()
{
}

int ReadBuffer::readSocketToBuffer(int fd)
{
#if 1
	int singleLen = -1;
	int totalLen = 0;
	//read需要处于非阻塞状态
	//如果刚好getWriteSize，就会返回0，然后误认为是断开了连接,所以有必要进行下面的判断
	pthread_mutex_lock(&m_wMutex);  //写锁加锁，注意是写到这个buf
	Debug("readSocketToBuffer加锁");
	if (getWriteSize() == 0) {
		resizeBuffer(1024);
	}
	//这个中断应该参考写的，也就是应该放到while里面，因为中断一般只有一次
	while ((singleLen = read(fd, m_data + m_writePos, getWriteSize()))>0) {
		m_writePos += singleLen;
		totalLen += singleLen;
		Debug("本次读到socket的数据大小为：%d,当前Buf的情况为：%s，当前读指针：%d，当前写指针：%d",
			singleLen, m_data, m_readPos, m_writePos);
		resizeBuffer(singleLen);
		Debug("尝试性扩容后（只是调用扩容接口）当前Buf的情况为：%s", m_data);
	}
	Debug("readSocketToBuffer解锁");
	pthread_mutex_unlock(&m_wMutex);  //写锁解锁 ，注意应该是while结束，才解锁，因为其他线程如果read就读错了
	if (singleLen == -1 && errno == EINTR) {
		Debug("singleLen == -1 && errno == EINTR");
	}
	else if (singleLen == -1 && errno == EAGAIN) {
		Debug("读数据完成，读到的字节数为：%d", totalLen);
		return totalLen;
	}else if (totalLen == 0) {
		printf("对方断开了连接\n");
		return 0;
	}
	else {
		perror("read");
	}
#else
	//并没有设置read和write为非阻塞
	/*
		阻塞式IO读取解决方案
		1、直接开辟一个很大的数组
		2、使用多个数组读取的方法，这个也是妥协的方法，先读取到一个小数组，如果数据很大，那么再读到一个较大的数组
		3、如果是自己实现的客户端可以先传输字节大小
	*/
#endif
	return totalLen;
}

int ReadBuffer::readSocketToBuffer(int fd, sockaddr_in* sockAddr, socklen_t* addrlen) {
	int pkgLen = -1;
	int totalLen = 0;
	int readSize = 0;
	//每次读之前需要接收缓冲区足够大
	Debug("当前Buf的情况为：%s，当前读指针：%d，当前写指针：%d",
		m_data, m_readPos, m_writePos);
	pthread_mutex_lock(&m_wMutex);  //写锁加锁，需要包含resizeBuf
	Debug("readSocketToBuffer2加锁");
	resizeBuffer(MSG_LEN_LIMIT);  //有一个隐患是这里默认包的长度是65535-head，不过可以在发出去的时候限定一下
	pkgLen = recvfrom(fd, m_data + m_writePos, getWriteSize(), 0, (sockaddr*)sockAddr, addrlen);
	if (pkgLen > 0) {
		m_writePos += pkgLen;
		//pthread_mutex_unlock(m_wMutex); //写锁解锁，不能在这解锁，万一进入其他分支就解不开了。。。。，所以下面不要有那么多return
		Debug("读到socket的数据大小为：%d,当前Buf的情况为：%s，当前读指针：%d，当前写指针：%d",
			pkgLen, m_data, m_readPos, m_writePos);
		readSize = pkgLen;
	}
	else  if (pkgLen == 0) {
		printf("对方断开了连接\n");
		readSize = 0;
	}
	else if (pkgLen == -1 && errno == EINTR) {
		std::cout << "当前读操作被中断，已忽略此包" << std::endl;
		readSize = -1;
	}
	//查看man文档，可以得到如下
	else if (pkgLen == -1 && (errno == EAGAIN || EWOULDBLOCK)) {
		std::cout << "当前不可读" << std::endl;
		perror("read");
		readSize = -1;
	}
	else {
		perror("read");
		readSize = -1;
	}
	Debug("readSocketToBuffer2解锁");
	pthread_mutex_unlock(&m_wMutex); //写锁解锁,解锁不能在if里面
	return readSize;
}

//======WriteBuffer=======
WriteBuffer::WriteBuffer() :Buffer() {}

WriteBuffer::WriteBuffer(int size) :Buffer(size)
{
}

WriteBuffer::~WriteBuffer() {}

int WriteBuffer::writeBufferToSocket(int fd) {
	if (getReadSize() <= 0) {
		//当前buf没有数据可写，应该返回1，假装写完，然后可以顺利禁止写事件
		//这个bug是改返回值的时候加totalLen的时候发现的
		//顺势又发现了在读socket的时候可能第一次可写的空间为0
		return 1;
	}
	int ret = -1;
	int totalLen = 0;
	//注意，这里一定要使用sendSize，来保证我当前writeBufferToSocket只发送sendSize字节，因为可能我在发的过程有其他进程在写，那么
	//write的函数如果用getReadSize就是错的
	//其实无所谓的，因为即使粘包也可以接收，只要不覆盖数据就行
	int sendSize = getReadSize();
	do {
		//大小一定是要getReadSize这个呀，否则出现奇怪的错误
		pthread_mutex_lock(&m_rMutex);  //读锁加锁，注意是读锁
		Debug("writeBufferToSocket加锁");
		ret = write(fd, m_data + m_readPos, sendSize);  // 一次性并不一定能写完
		//getReadSize如果这个值为0会返回0，但是有了上面的if，这个值肯定不会为0
		
		
	} while (ret == -1 && errno == EINTR);  
	//while (ret == -1 && (errno == EAGAIN || errno == EINTR)); //(errno == EAGAIN 表示写满了，下次在尝试
	//我这个如果等于errno == EAGAIN会不停的循环，其实是不高效的，而是直接break，返回0代表当前不可写，
	//等可写的时候你再告诉我，而我这个会一直循环
	if (ret == -1 && errno == EAGAIN) {
		//说明当前fd的缓冲区满了，写不进去，先退出
		Debug("writeBufferToSocket解锁");
		pthread_mutex_unlock(&m_rMutex);  //读锁解锁 ，这里也要解锁。。。
		return 0; //暂时不可写，下次再写
	}
	if (ret >= 0) {
		m_readPos += ret;
		Debug("writeBufferToSocket解锁");
		pthread_mutex_unlock(&m_rMutex);  //读锁解锁
		totalLen += ret;
		Debug("本次写到socket的字节数为：%d", ret);
	}
	return totalLen;
	/*
	do {
		//大小一定是要getReadSize这个呀，否则出现奇怪的错误
		ret = write(fd, m_data + m_readPos, getReadSize());  // 一次性并不一定能写完
		if (ret >= 0) {
			m_readPos += ret;
		}
	} while (ret == -1 && errno == EINTR);这种适合阻塞的fd也适合非阻塞的fd，EINTR表示非阻塞失败
	*/
}

int WriteBuffer::writeBufferToSocket(int fd, int pkgLen, sockaddr_in* sockAddr, socklen_t addrlen) { //之所以加这个msglen是因为可能在在函数体内获取的长度大于一个包长度，因为可能其他线程也写
	if (pkgLen <= 0) {
		return 1;
	}
	int ret = -1;
	int totalLen = 0;
	do {
		Debug("正在发消息给：%s:%d", inet_ntoa(sockAddr->sin_addr), ntohs(sockAddr->sin_port));
		pthread_mutex_lock(&m_rMutex);  //读锁加锁
		Debug("writeBufferToSocket2加锁");
		ret = sendto(fd, m_data + m_readPos, pkgLen, 0, (sockaddr*)sockAddr, addrlen);  // 一次性并不一定能写完
		//getReadSize如果这个值为0会返回0，但是有了上面的if，这个值肯定不会为0

	} while (ret == -1 && errno == EINTR);
	//while (ret == -1 && (errno == EAGAIN || errno == EINTR)); //(errno == EAGAIN 表示写满了，下次在尝试
	//如果等于errno == EAGAIN会不停的循环，其实是不高效的，而是直接break，返回0代表当前不可写，
	//等可写的时候你再告诉我，而我这个会一直循环
	if (ret == -1 && errno == EAGAIN) {
		//说明当前fd的缓冲区满了，写不进去，先退出
		Debug("writeBufferToSocke2t解锁");
		pthread_mutex_unlock(&m_rMutex);  //读锁解锁，这个分支也要解锁
		return 0; //暂时不可写，下次再写
	}
	if (ret >= 0) {
		m_readPos += ret;
		Debug("writeBufferToSocket2解锁");
		pthread_mutex_unlock(&m_rMutex);  //读锁解锁
		totalLen += ret;
		Debug("本次写到socket的字节数为：%d", ret);
	}
	return totalLen;
}
