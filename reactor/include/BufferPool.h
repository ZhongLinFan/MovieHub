#pragma once
#include <pthread.h>
#include <map>

struct MUnit {
public://设计成公有，可以很方便的在BufferPool（所有的成员都要用到）和Buffer（用到m_data和m_capacity）里传递信息，
	MUnit(int capacity) {
		m_data = new char[capacity];
		m_capacity = capacity;
	}
	char* m_data;
	MUnit* m_next;
	int m_capacity;
};

enum class MScale:int
{
	m1k = 1024,
	m2k = 2048,
	m4k = 4096,
	m8k = 8192,
	m16k = 16384,
	m32k = 32768,
	m64k = 65536,
	m128k = 131072,
	m256k = 262144,
	m512k = 524288,
	m1m = 1048576,
	m2m = 2097152,
};

class BufferPool {
public:
	//=======单例======
	//这两个也可以设置为私有
	BufferPool(const BufferPool& pool) = delete;
	BufferPool& operator=(const BufferPool& pool) = delete;
	static void init() {
		m_instance = new BufferPool();
	}
	static BufferPool* getInstance() {
		pthread_once(&m_mutex, init); // 如果没有执行过，那么会执行init方法，如果执行过，那么当前语句不执行
		return m_instance; //注意要分清m_instance和m_pool，m_instance相当于一个具有完整功能的内存池，而m_pool只相当于存放数据的地方，没有其他功能
	}

	//=======功能=======
	//获取一个内存块,从map中取出一个内存块
	MUnit* allocm() { return allocm((int)MScale::m4k); };
	MUnit* allocm(int size);
	//回收一个内存块
	bool recycle(MUnit* buf);
private:
	//BufferPool以及allocm中当前刻度内存用完之后，循环开辟内存块的辅助函数,返回实际开辟的内存块数量，可能超过内存池上限，就中止了
	int allocUnits(MScale size, int nums);

/*
* 错误架构下的产物
private:
	//访问Buffer指针
	Buffer* getBufferNext(Buffer* buf);
*/

private:
	//=======单例======
	BufferPool();
	static BufferPool* m_instance;
	//静态锁，用于保证某方法只被执行一次
	static pthread_once_t m_mutex;
	//=======功能======
	//存放所有内存块的句柄
	std::map<MScale, MUnit*> m_pool;
	//标记当前内存块是否可用，因为可能当前系统资源用的太多（或者超过上限），开辟不出来了，然后某一个刻度又没有了
	std::map<MScale, bool> m_available;
	//内存池当前实际容量
	uint64_t m_totalSize;  // 单位也是kb 
	const uint64_t m_litmit = 5 * 1024 * 1024; //上限值 ，以k为单位.上限值为5G
};