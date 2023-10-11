#include "BufferPool.h"
#include <iostream>
#include <assert.h>
//注意静态变量初始化的方式
BufferPool* BufferPool::m_instance = nullptr;
pthread_once_t BufferPool::m_mutex = PTHREAD_ONCE_INIT;


BufferPool::BufferPool()
{
	m_totalSize = 0;
	//每个都是10Mb，初始值，所以当前占用内存初始值为120MB
	//不用初始化m_available，在allocUnits内部已经初始化过了

	//注意一定要初始化m_pool,由于不连续的枚举无法遍历，所以直接赋值
	m_pool[MScale::m1k] = nullptr;
	m_pool[MScale::m2k] = nullptr;
	m_pool[MScale::m4k] = nullptr;
	m_pool[MScale::m8k] = nullptr;
	m_pool[MScale::m16k] = nullptr;
	m_pool[MScale::m32k] = nullptr;
	m_pool[MScale::m64k] = nullptr;
	m_pool[MScale::m128k] = nullptr;
	m_pool[MScale::m256k] = nullptr;
	m_pool[MScale::m512k] = nullptr;
	m_pool[MScale::m1m] = nullptr;
	m_pool[MScale::m2m] = nullptr;

	allocUnits(MScale::m1k, 10240); //10Mb
	allocUnits(MScale::m2k, 5120); //10Mb
	allocUnits(MScale::m4k, 2560); //10Mb
	allocUnits(MScale::m8k, 1280);
	allocUnits(MScale::m16k, 640);
	allocUnits(MScale::m32k, 320);
	allocUnits(MScale::m64k, 160);
	allocUnits(MScale::m128k, 80);
	allocUnits(MScale::m256k, 40);
	allocUnits(MScale::m512k, 20); //10mb
	allocUnits(MScale::m1m, 10); // 10mb
	allocUnits(MScale::m2m, 5); //10mb
}

MUnit* BufferPool::allocm(int size)
{
	MScale scale = MScale::m1k;
	if(size < (int)MScale::m1k && m_available[MScale::m1k] == true){ // 小于0的默认也给他开辟这么大空间
		scale = MScale::m1k;
	}
	else if (size < (int)MScale::m2k && m_available[MScale::m2k] == true) {
		scale = MScale::m2k;
	}
	else if (size < (int)MScale::m4k && m_available[MScale::m4k] == true) {
		scale = MScale::m4k;
	}
	else if (size < (int)MScale::m8k && m_available[MScale::m8k] == true) {
		scale = MScale::m8k;
	}
	else if (size < (int)MScale::m16k && m_available[MScale::m16k] == true) {
		scale = MScale::m16k;
	}
	else if (size < (int)MScale::m32k && m_available[MScale::m32k] == true) {
		scale = MScale::m32k;
	}
	else if (size < (int)MScale::m64k && m_available[MScale::m64k] == true) {
		scale = MScale::m64k;
	}
	else if (size < (int)MScale::m128k && m_available[MScale::m128k] == true) {
		scale = MScale::m128k;
	}
	else if (size < (int)MScale::m256k && m_available[MScale::m256k] == true) {
		scale = MScale::m256k;
	}
	else if (size < (int)MScale::m512k && m_available[MScale::m512k] == true) {
		scale = MScale::m512k;
	}
	else if (size < (int)MScale::m1m && m_available[MScale::m1m] == true) {
		scale = MScale::m1m;
	}
	else if (size < (int)MScale::m2m && m_available[MScale::m2m] == true) {
		scale = MScale::m2m;
	}
	else {
		std::cout << "需要的空间太多，无法满足要求" << std::endl;
		return nullptr; //要求的内存空间太大，或者当前内存块全部分配出去，并且已经不能再申请了，就返回空
	}
	//到这，能确保一定可以拿到一块有效可以分配的内存，但是如果这个刻度只剩一块，那么就要提前为其分配，或者更新状态
	//注意，MScale类型的参数从外部传进来需要经过int->MScale（scale）的转换过程（转换的时候MScale还需要转换为int）,m_pool的key是MScale
	MUnit* target = m_pool[scale];
	assert(target); // 断言当前一定不为空
	m_pool[scale] = target->m_next;  //这个可以提前进行的，因为反正下面要继续初始化
	//一定要记得断开target尾指针
	target->m_next = nullptr;
	if (m_pool[scale] == nullptr){ //如果取的是当前刻度的最后一块内存了
		if (!allocUnits(scale, 10)) { //一个scale的内存块都开辟不出来了，说明当前系统开辟的内存块太多了（或者已经达到自定义的上限值）
			m_available[scale] = false; //如果开辟不出来，就标记不可用
		};
		//如果能开辟出来，m_pool[scale]就不为空了， 那么下次就可以继续使用当前刻度了
	}
	//返回已经取出的内存块
	return target;
}

bool BufferPool::recycle(MUnit* buf)
{
	//assert(buf && buf->m_capacity == );这里想断言，但是不会表达，还是对map不熟悉
	MScale cap = (MScale)buf->m_capacity;  //recycle同样也需要int-》MScale的一个转换
	assert(buf && buf->m_data && m_pool.find(cap) != m_pool.end());
	//不需要再重置了，上面assert的信息足矣
	
	//访问共享资源需要加锁,头插操作
	MUnit* tmp = m_pool[cap];
	m_pool[cap] = buf;
	buf->m_next = tmp;

	m_available[cap] = true; //回收一块内存及时更新状态
	return true;
}

int BufferPool::allocUnits(MScale size, int nums)
{
	//记得加锁
	//记得统计实际占用字节
	if (m_totalSize >= m_litmit) {
		//注意这里是size
		m_available[size] = false; //如果当前节点已经空了，但是又达到上限了，只能设置为false，这里很容易漏掉
		return 0;  //如果超过上限值
	}
	assert(m_pool.find(size) != m_pool.end() && m_pool[size] == nullptr); // 因为都是链表为空时，才开始扩充，所以进行一下断言
	m_available[size] = true; //不管以前有没有更新，到这肯定可以用了
	m_pool[size] = new MUnit((int)size);
	m_totalSize += (int)size / 1024;
	MUnit* buf = m_pool[size];
	int i = 0;
	for (; i < nums && m_totalSize < m_litmit; i++) {
		buf->m_next = new MUnit((int)size);
		buf = buf->m_next;
		m_totalSize += (int)size / 1024;
	}
	buf->m_next = nullptr;
	return i + 1; //1是头节点，到这肯定开辟了
}

/*
//友元函数
Buffer* BufferPool::getBufferNext(MUnit* buf)
{
	return buf->m_next;
}
*/
