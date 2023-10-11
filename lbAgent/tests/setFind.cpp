#include <set>
#include <iostream>
#include <string>

struct HostInfo {
	std::string ip;
	int port;
	int serviceCondition;
	bool operator==(const HostInfo& obj) const {
		std::cout << "operator==比较信息：" << "ip:" << ip << "obj.ip:" << obj.ip << "port:" << port << "obj.port:"
			<< obj.port << "serviceCondition:" << serviceCondition << "obj.serviceCondition :" << obj.serviceCondition << std::endl;
		return ip == obj.ip && port == obj.port;
	}
};

class HostServiceConditionCmp {
public:
	bool operator()(const HostInfo& left, const HostInfo& right) {
#if 0
		if (left.ip != right.ip) {
			return left.ip < right.ip;
		}
		if (left.port != right.port) {
			return left.port < right.port;
		}
		//到这说明两个相等了，这个时候应该按照serviceCondition排序，但是其实这时候不应该插入的，这相当于是个悖论
		//说明
		return left.serviceCondition < right.serviceCondition;
#endif
#if 0
		if (left.ip != right.ip) {
			return left.serviceCondition < right.serviceCondition;
		}
		if (left.port != right.port) {
			return left.serviceCondition < right.serviceCondition;
		}
		//到这说明两个相等了，但是其实这时候不应该插入的,所以正确的就应该这么写
		//但是一这样写，又回到了原来的问题，就是查找只查找第一个就结束了，就会导致查找异常（但是解决了之前端口和ip相等时，只能借助mysql主键约束来规避
		//如果外面传入两个相同ip和端口，依然能够插入，但是这里就解决了这个问题）
		//这个时候就该考虑给查找单独定义一个函数了。查找函数单独写
		//不行，这个回导致temp永远无法使用find了。。。。。。。
		return false;
#endif
		if (left == right) {
			return left.serviceCondition < right.serviceCondition;
		}
		return left.serviceCondition <= right.serviceCondition;
	}
};



class SetFindTest {
public:
	SetFindTest() {}
	void insertElement() {
		HostInfo host1;
		host1.ip = "127.0.0.1";
		host1.port = 10011;
		host1.serviceCondition = 1;
		HostInfo host2;
		host2.ip = "127.0.0.1";
		host2.port = 10010;
		host2.serviceCondition = 2;
		temp.insert(host1);
		temp.insert(host2);
		std::cout << "temp.size():" << temp.size() << std::endl;
		std::cout << "基本信息：" << std::endl;
		for (const auto& ele: temp) {
			std::cout << "ip:" << ele.ip << "\tport:" << ele.port << "\tserviceCondition:" << ele.serviceCondition << std::endl;
		}
	}
	void findElement() {
		HostInfo host1;
		host1.ip = "127.0.0.1";
		host1.port = 10010;
		host1.serviceCondition = 0;
		HostInfo host2;
		host2.ip = "127.0.0.1";
		host2.port = 10011;
		host2.serviceCondition = 0;
		auto target1 = temp.find(host1);
		if (target1 != temp.end()) {
			std::cout << "第一次查找成功" << std::endl;
		}
		else {
			std::cout << "第一次查找失败" << std::endl;
		}

		auto target2 = temp.find(host2);
		if (target2 != temp.end()) {
			std::cout << "第二次查找成功" << std::endl;
		}
		else {
			std::cout << "第二次查找失败" << std::endl;
		}
	}
public:
	std::set<HostInfo, HostServiceConditionCmp> temp;
};

int main() {
	SetFindTest test;
	test.insertElement();
	test.findElement();
	return 0;
}