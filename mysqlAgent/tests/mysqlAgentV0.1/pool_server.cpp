#include "ConnectPool.h"
#include <iostream>
#include <chrono>
#include <thread>

//#define NOT_POOL

//单线程非连接池
void singleThread_Not(int begin, int end) {
	for (int i = begin; i < end; i++) {
		MysqlConn* conn = new MysqlConn();
		conn->connect("root", "1234", "movie_hub", "127.0.0.1");
		std::string sql = "select * from router";
		bool res = conn->query(sql);
		if (!res) {
			std::cout << i << std::endl;
		}
		delete conn;
	}
}

//单线程连接池
void singleThread_Yes(ConnectPool* pool, int begin, int end) {
	for (int i = begin; i < end; i++) {
		MysqlConn* conn = pool->getConnect();
		std::string sql = "select * from router";
		bool res = conn->query(sql);
		if (!res) {
			std::cout << i << std::endl;
		}
		pool->putBackConnect(conn);
	}
}

void singleThread() {
#ifdef NOT_POOL
	//测试单线程非连接池查询数据
	auto start = std::chrono::steady_clock::now();
	singleThread_Not(0, 10000);
	auto end = std::chrono::steady_clock::now();
	//计算多少纳秒
	//std::chrono::steady_clock::duration<long long, std::nano> len = end - start;  //等价于std::chrono::nanoseconds
	//上面是错的
	std::chrono::duration<long long, std::nano> len = end - start;  //等价于std::chrono::nanoseconds
	//转为毫秒
	std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(len);
	std::cout << "单线程，非连接池用时：" << millsec.count() << "ms\t" << millsec.count() / 1000 << "s" << std::endl;
#else
	//测试单线程连接池查询数据
	ConnectPool* pool = ConnectPool::getConnectPool("movie_hub");
	auto start = std::chrono::steady_clock::now();
	singleThread_Yes(pool, 0, 100000);
	auto end = std::chrono::steady_clock::now();
	//计算多少纳秒
	std::chrono::duration<long long, std::nano> len = end - start;  //等价于std::chrono::nanoseconds
	//转为毫秒
	std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(len);
	std::cout << "单线程，连接池用时：" << millsec.count() << "ms\t" << millsec.count() / 1000 << "s" << std::endl;
#endif
}

//多线程访问mysql
void multiThread() {
#ifdef NOT_POOL
	//测试多线程非连接池查询数据
	auto start = std::chrono::steady_clock::now();
	std::thread* t[5];
	for (int i = 0; i < 5; i++) {
		t[i] = new std::thread(singleThread_Not, 0, 2000);
		t[i]->join();
	}
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<long long, std::nano> len = end - start; 
	std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(len);
	std::cout << "多线程，非连接池用时：" << millsec.count() << "ms\t" << millsec.count() / 1000 << "s" << std::endl;
#else
	//测试多线程使用连接池同时访问数据库查询数据
	ConnectPool* pool = ConnectPool::getConnectPool("movie_hub");
	auto start = std::chrono::steady_clock::now();
	std::thread* t[5];
	for (int i = 0; i < 5; i++) {
		t[i] = new std::thread(singleThread_Yes, pool,  0, 20000);
		t[i]->join();
	}
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<long long, std::nano> len = end - start;
	std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(len);
	std::cout << "多线程，连接池用时：" << millsec.count() << "ms\t" << millsec.count() / 1000 << "s" << std::endl;
#endif
}

int main(int argc, void* arg[])
{
#if 0
	//1、MysqlConn类
	MysqlConn* conn = new MysqlConn();
	if (conn->connect("root", "1234", "movie_hub", "127.0.0.1")) {
		std::cout << "连接成功" << std::endl;
	}
	else {
		std::cout << "抱歉，连接失败" << std::endl;
	}
	//插入
	std::string sql = "insert into router values(NULL, 2, 9999,10010)";
	bool res = conn->update(sql);
	std::cout << "update:" << res << std::endl;
	//查询
	sql = "select * from router";
	res = conn->query(sql);
	std::cout << "query:" << res << std::endl;
	//打印查询到的结果集
	while (conn->next()) {
		std::cout << conn->value(0) << "," << conn->value(1) << "," << conn->value(2) << "," << conn->value(3) << std::endl;
	}
	
#elif 0
	//测试连接池----单线程
	singleThread();
#elif 1
	//测试连接池----多线程
	multiThread();
#endif
}
