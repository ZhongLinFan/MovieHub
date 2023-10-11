#include "ConnectPool.h"
#include <thread>

MysqlConn::MysqlConn()
{
	m_conn = mysql_init(nullptr);
	//接口编码设置为utf8，也就是这个接口的编码默认不是utf8，如果读中文，编码不是utf8有可能会乱码
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	if (m_conn != nullptr) {
		mysql_close(m_conn);
	}
	//断开连接尝试释放结果集数据
	freeResult();
}

bool MysqlConn::connect(std::string user, std::string passwd, std::string dbName, std::string ip, unsigned short port)
{
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	//m_IdleTime = std::chrono::steady_clock::now();
	refreshLastTime();
	return ptr != nullptr;
}

bool MysqlConn::update(std::string sql)
{
	if (mysql_query(m_conn, sql.c_str()) == 0) {
		return true;
	}
	return false;
}

bool MysqlConn::query(std::string sql)
{
	freeResult(); //每次查询之前都清空上次的数据
	if (mysql_query(m_conn, sql.c_str()) == 0) {
		m_result = mysql_store_result(m_conn);
		return true;
	}
	return false;
}

bool MysqlConn::next()
{
	if (m_result != nullptr) {
		m_row = mysql_fetch_row(m_result);
		//如果没有这条语句，你打印不出来东西，因为没有返回true
		if (m_row != nullptr) {
			return true;
		}
	}
	return false;
}

std::string MysqlConn::value(int index)
{
	int colCount = mysql_num_fields(m_result);
	if (index > colCount || index < 0) {
		return std::string();
	}
	char* val = m_row[index];
	//如果val包含二进制数据，那么可能含有\0,那么char*转为string的时候就有问题，需要以下处理
	unsigned long length = mysql_fetch_lengths(m_result)[index]; // mysql_fetch_lengths(m_result)返回的是结果集中的当前记录所有字段的值的长度
	return std::string(val, length);
}

bool MysqlConn::transaction()
{
	return mysql_autocommit(m_conn, false);
}

bool MysqlConn::commit()
{
	return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshLastTime()
{
	m_IdleTime = std::chrono::steady_clock::now();  //now得到能够精确到纳秒
}

long long MysqlConn::getIdleTime()
{
	//chrono提供了以纳秒为单位的数据类型
	std::chrono::nanoseconds res = std::chrono::steady_clock::now() - m_IdleTime;
	//可以转换为精度比较低的单位
	std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
	//millsec是对象，如果想要知道有多少个毫秒，需要调用其中的方法
	return millsec.count();
}

void MysqlConn::freeResult()
{
	if (m_result) {
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}

ConnectPool* ConnectPool::getConnectPool(std::string dbName)
{
	//注意静态局部变量只在当前作用域可见，但是生命周期是全局的，也就是进程结束之前都会存在（在全局区）
	//静态局部变量有一个特点就是在编译时赋初值，运行时每次调用这个函数，都不在重新赋值，而是保留上次函数调用结束的值
	//（比如static y = x,如果第一次x为5，那么第一次调用时y为5，第二次调用时x为10，但是第二次执行到这后，y依然时5，x为10）
	//另一个重要的特点时c++11之后，静态成员变量（网上查到的时静态局部变量）是线程安全的，是编译器保证的，也就是如果多个线程同时进入声明语句
	//static ConnectPool pool;，那么只有一个进程会成功，其他进程都会阻塞，那么其他进程再执行就不会再创建了
	static ConnectPool pool(dbName);
	return &pool;
}

MysqlConn* ConnectPool::getConnect()
{
	//先判断是否还有可用的连接
	//事先准备好互斥锁
	std::unique_lock<std::mutex> locker(m_mutexQ);
	//if (m_connectQ.empty()) {
	//这里必须使用while，而不是if，因为消费者线程在消费一个后会唤醒所有的消费者和生产者，所以当消费者被唤醒的时候还需要在这里进行循环回来再判断一下是否空
	while (m_connectQ.empty()) {
		//让当前的线程阻塞
		//注意唤醒可能有两种可能一种是阻塞到时间用完自己唤醒了，还有一种是其他线程唤醒了当前的条件变量（notify_one或者notify_all），这里就有必要分清是那种
		//注意如阻塞在这，说明当前消费不了连接了，连接已经用完了，这个时候如果生产者生产了线程，应该通知阻塞在这的线程可以消费连接了
		if (std::cv_status::timeout == m_cond.wait_for(locker, std::chrono::milliseconds(m_timeout))) {
			//如果是到这，说明队列还是为空，不过为了保险起见还是判断一下
			if (m_connectQ.empty()) {
				return nullptr;
				//博主在这改成继续等待，这里改成了continue，然后最外层改成了while，我这里没有
			}
		}
	}
	//到这说明有可用连接,注意这里一直是被锁着的
	MysqlConn* conn = m_connectQ.front();
	m_connectQ.pop();
	//注意到这个地方说明已经消费了可用连接，这个时候需要唤醒生产者进程，让生产者线程判断是否该生产
	m_cond.notify_all();   //注意这个m_cond既阻塞生产者又阻塞消费者，如果唤醒的话，唤醒的是两类，一类是生产者，一类是消费者，但是这里实际上只想唤醒生产者
	//所以，可以考虑使用while，而不是if或者生产者和消费者使用不同的条件变量
	return conn;
}

void ConnectPool::putBackConnect(MysqlConn* conn)
{
	//博主是使用智能指针的东西
	if (conn) {
		//操作共享资源需要加锁，一种是使用lock和unlock方法，一种是是使用luck_guard
		std::lock_guard<std::mutex> locker(m_mutexQ);
		conn->refreshLastTime();
		m_connectQ.push(conn);
	}
}

ConnectPool::~ConnectPool()
{
	while (!m_connectQ.empty())
	{
		MysqlConn* conn = m_connectQ.front();
		m_connectQ.pop();
		delete conn;
	}
}

ConnectPool::ConnectPool(std::string dbName)
{
	m_ip = "127.0.0.1";
	m_user = "root";
	m_passwd = "1234";
	m_dbName = dbName;
	m_port = 3306;
	m_minSize = 50;
	m_maxSize = 100;
	m_timeout = 1000; //1000ms
	m_MaxIdleTime = 5000; //5000ms
	//创建数据库连接
	for (int i = 0; i < m_minSize; ++i) {
		addConnect();
	}
	//连接过少时，负责创建连接的线程
	std::thread producer(&ConnectPool::produceConnect, this);
	//连接太多时，负责减少连接的线程
	std::thread recycler(&ConnectPool::recycleConnect, this);

	//当前是主线程在执行，主线程还有其他任务在做，所以不能使用join阻塞，等待上面两个函数执行完
	//所以让主线程和子线程分离，就是detach函数
	producer.detach();
	recycler.detach();
}

void ConnectPool::produceConnect()
{
	//while true是保证这个线程永远有活干，因为不加的话，当前函数执行一下后续就不会执行了
	while (true)
	{
		std::unique_lock<std::mutex> locker(m_mutexQ);  //不需要手动加锁和解锁了，这个交给locker进行的，当locker创建之后自动加锁，当析构的时候自动解锁，至于什么时候被析构，就是出了第一层while的大括号就被析构了
		//这个while是条件变量
		while (m_connectQ.size() >= m_minSize) {
			//当个数够的时候，就阻塞休眠
			m_cond.wait(locker);
		}
		//当小于个数的时候就需要创建数据库连接
		addConnect();
		m_cond.notify_one();  //生成完需要唤醒阻塞在等待获取连接的线程，因为只生产一个嘛，所以唤醒一个
		//注意这里只会唤醒消费者线程，因为只有一个生产者线程，也就是n个消费者线程和1个生产者线程都使用一个cond，
		//如果是多个生产者线程，那么就会唤醒生产者和消费者，但是即使是多个生产者，当前线程唤醒另外的生产者也是没有影响的，因为唤醒之后，会再通过while判断一下
		//不需要解锁了
	}
}

void ConnectPool::recycleConnect()
{
	while (true) {
		//为了保证性能，可以让这个函数一定周期执行以下，也就是休眠一段时间唤醒检测一下
		std::this_thread::sleep_for(std::chrono::seconds(10));  //休眠1秒
		//这个线程的任务就是监视空闲连接是否太多，比如起始是30个连接，突然一个高峰（只有申请没有归还），就会导致produceConnect每次检测都会
		//小于 m_minSize，然后就创建连接（当然也有可能生产者线程没有抢到时间片，只要他抢到时间片就会让数量保持在最小值上），当高峰过了之后
		//有必要销毁空闲的连接，那么销毁多少呢，这个就有必要引入每个连接的时间戳了，因为我可不想高峰期的时候就销毁，或者高峰期一过就销毁
		// 而是高峰期过了，让这些连接闲置一定时间之后，还没有被使用，那我就认为是空闲连接了当然了这个是定时的检测
		// 当大于最大连接数的时候是一定要回收的
		while (m_connectQ.size() >= m_maxSize)
		{
			std::lock_guard<std::mutex> locker(m_mutexQ);
			delConnect();
		}
		//大于最小连接数才需要销毁，注意当前队列里面全都是空闲的
		//如果大于的话，并不会回收所有的连接，而是回收闲置一定时长的连接
		while (m_connectQ.size() >= m_minSize) {
			//访问共享资源，加锁
			std::lock_guard<std::mutex> locker(m_mutexQ);
			MysqlConn* conn = m_connectQ.front();
			if (conn->getIdleTime() > m_MaxIdleTime) {
				delConnect();
			}
		}
	}
}

void ConnectPool::addConnect()
{
	MysqlConn* conn = new MysqlConn;
	conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
	m_connectQ.push(conn);
}

void ConnectPool::delConnect()
{
	//取对头的元素销毁，为啥呢，因为队列是先进先出，如果空闲的太多了，那么肯定是最早空闲的
	MysqlConn* conn = m_connectQ.front();
	//博主还在这加了一个判断是否大于最小时长的逻辑
	m_connectQ.pop();
	delete conn;
}
