#pragma once
#include <string>
#include "mysql.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <string>

class MysqlConn {
public:
	//初始化数据库连接
	MysqlConn();
	//释放数据库连接
	~MysqlConn();
	//连接数据库
	bool connect(std::string user, std::string passwd, std::string dbName, std::string ip, unsigned short port=3306);
	//更新数据库 insert，update，delete
	bool update(std::string sql);
	//查询数据库
	bool query(std::string sql);
	//遍历查询得到的结果集
	bool next();
	//得到结果集中的字段值
	std::string value(int index);
	//事务操作
	bool transaction();
	//提交事务
	bool commit();
	//事务回滚
	bool rollback();

	//刷新当前线程的最后使用的时间戳
	void refreshLastTime();
	//获得空闲时长
	long long getIdleTime();
private:
	void freeResult();
public:
	MYSQL* m_conn = nullptr;
	MYSQL_RES* m_result = nullptr;
	MYSQL_ROW m_row = nullptr;  //MYSQL_ROW本身就是个指针类型
	//使用c++的绝对时钟，当然c的time函数及difftime也是可以用的
	std::chrono::steady_clock::time_point m_IdleTime;
};

class ConnectPool {
public:
	//通过类名获取实例对象，那么就需要静态成员函数和静态成员变量
	static ConnectPool* getConnectPool(std::string dbName);

	//防止其他方式复制已经得到的实例对象
	ConnectPool(const ConnectPool& obj) = delete;
	ConnectPool& operator=(const ConnectPool& obj) = delete;

	//外部获取连接的方法
	MysqlConn* getConnect();
	//外部放回用完的连接
	void putBackConnect(MysqlConn* conn);
	//释放资源
	~ConnectPool();
private:
	ConnectPool(std::string dbName);  //设置为私有，外界不能谁便访问

	//创建连接的函数
	void produceConnect();
	void recycleConnect();
	void addConnect();  //添加连接的动作
	void delConnect();  //删除连接的动作
private:
	std::queue<MysqlConn*> m_connectQ;  //这只是连接池的一个属性，而不是连接池
	//连接数据库需要的属性
	std::string m_ip;
	std::string m_user;
	std::string m_passwd;
	//std::map<int, stirng> m_dbNameList; //注意这个int代表当前表的连接个数
	//上面是错的，三张表都在一个数据库里，直接string就可以
	std::string m_dbName;
	unsigned short m_port;
	//数据库连接的尺寸限制
	int m_minSize;
	int m_maxSize;

	//客户端获取连接最长等待时间
	int m_timeout;
	//连接最长空闲时长
	int m_MaxIdleTime;
	//多线程同时访问共享资源的互斥锁
	std::mutex m_mutexQ;
	//生产者和消费者线程需要的同步变量
	std::condition_variable m_cond;

};