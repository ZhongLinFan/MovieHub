#include <iostream>
#include <set>
#include <unordered_map>

class ServerAddr{
        public:
                ServerAddr(){
                        m_token = rand()%100000;
                        std::cout << "无参构造ServerAddr" <<std::endl;
                }
                ServerAddr(std::string ip, short port):ServerAddr(){
                        m_ip = ip;
                        m_port = port;
                }
                bool operator==(const ServerAddr& server)const {  //必须要有const，否则会报错
                        return server.m_ip == m_ip &&  server.m_port == m_port;
                }
        public:
                std::string m_ip;
                short m_port;
                int m_token;
};

struct serviceCondition{
        public:
                serviceCondition(){
                        std::cout << "无参构造serviceCondition" <<std::endl;
                }
                serviceCondition(std::string ip, short port, int nums){
                        std::cout << "有参构造serviceCondition" <<std::endl;
                        m_addr.m_ip = ip;
                        m_addr.m_port = port;
                        m_nums = nums;
                }
                serviceCondition(const serviceCondition& obj){  //必须要加const，否则无法转换会报错
                        m_nums = obj.m_nums;
                        //obj.nums = nums;不对的
                        std::cout << "拷贝构造serviceCondition" <<std::endl;
                }
                //注意，移动构造前后依然是两个对象，只不过如果原来的对象里有堆内存的空间，使用移动构造，可以使用指针赋值的方式转移堆内存空间，前后依然要创建两个对象
                //所以类对象中都是栈上的话，没有区别
                serviceCondition(serviceCondition&& obj){   //如果不提供这个，那么std::move没用，依然会调用拷贝构造，如果提供了，需要提供operator=，因为如果提供移动，那么operator=会被标记为delete
                        m_nums = obj.m_nums;
                        std::cout << "移动构造serviceCondition" <<std::endl;
                }
                serviceCondition& operator=(const serviceCondition& obj){  
                        m_nums = obj.m_nums;
                        std::cout << "赋值serviceCondition" <<std::endl;
                        return *this;
                }
                ~serviceCondition(){
                        std::cout << "正在析构serviceCondition" << "nums:" << m_nums << std::endl;
                }
        public:
                ServerAddr m_addr;
                int m_nums;
};

//给mutliset用的
class ptrCmp{
        public:
                bool operator()(const serviceCondition* left, const serviceCondition* right){//const serviceCondition* &会报错
                        return left->m_nums < right->m_nums;
                }
};

//给unordered_map用的
class getHashCode{
        public:
                std::size_t operator()(const ServerAddr& server) const{ //必须要有const，否则会报错
                        //生成ip的hash值
                        std::size_t h1 = std::hash<std::string>()(server.m_ip);
                        //生成port的hash值
                        std::size_t h2 = std::hash<short>()(server.m_port);
                        //生成token的hash值
                        //std::size_t h3 = std::hash<int>()(server.m_token);//不能使用随机数，否则你根本找不到你的数据
                        //使用异或运算降低碰撞(相同为为0，相异为1)
                        return h1 ^ h2 ;
                }
};

bool insertSetElem(std::unordered_map<ServerAddr, serviceCondition*, getHashCode>&serverMap,
                std::multiset<serviceCondition*,ptrCmp>& serverSet, serviceCondition* condition){
        ServerAddr serverAddr = ServerAddr(condition->m_addr); //拷贝构造
        std::cout << "serverAddr:" << serverAddr.m_ip << "   "<< serverAddr.m_port << std::endl;
        if(serverMap.find(serverAddr) != serverMap.end()){
                std::cout << "相同server，无法插入" << std::endl;
                return false;
        }
        serverSet.insert(condition);
        serverMap.emplace(serverAddr, condition);  //insert会出错
        return true;
}

int main()
{
        std::multiset<serviceCondition*,ptrCmp> routerSet;
        std::unordered_map<ServerAddr, serviceCondition*, getHashCode>serverMap;
        serviceCondition* server1 = new serviceCondition("127.0.0.1", 10000, 1);
        serviceCondition* server2 = new serviceCondition("127.0.0.1", 10001, 1);
        serviceCondition* server3 = new serviceCondition("127.0.0.1", 10001, 2);
        std::cout << "set中key和unordered_map中value为指针测试开始" << std::endl;
        insertSetElem(serverMap, routerSet, server1);
        insertSetElem(serverMap, routerSet, server2);
        insertSetElem(serverMap, routerSet, server3);

        for(auto key : routerSet){
                std::cout <<"routerSet" <<  key->m_nums << std::endl;
        }
        for(auto key : serverMap){
                std::cout <<"serverMap" <<  key.first.m_ip <<  key.first.m_port << std::endl;
        }
        std::cout << "set中key和unordered_map中value为指针测试结束" << std::endl;
}

