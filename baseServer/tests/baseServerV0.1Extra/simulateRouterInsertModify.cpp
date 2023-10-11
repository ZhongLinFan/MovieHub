#include <iostream>
#include <set>
#include <unordered_map>
#include <cmath>
#include <random>

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

struct serviceCondition; //需要前置声明一下
//给mutliset用的
//需要放在serviceCondition上面
class ptrCmp{
        public:
                bool operator()(const serviceCondition* left, const serviceCondition* right){//const serviceCondition* &会报错
                        return left < right;
                }
};

struct serviceCondition{
        public:
                serviceCondition(std::string ip, short port, int nums, const std::multiset<serviceCondition*,ptrCmp>& routerSet){
                        weight["m_clientNums"] = 1;
                        m_clientNums = 0;
                        m_initalCondition = getInitialCondition(routerSet);
                        m_condition = m_initalCondition;
                        refreshCondition();
                        std::cout << "有参构造serviceCondition" <<std::endl;
                        m_addr.m_ip = ip;
                        m_addr.m_port = port;
                        m_clientNums = nums;
                }
                serviceCondition(const serviceCondition& obj){  //必须要加const，否则无法转换会报错
                        //m_nums = obj.m_nums;
                        //obj.nums = nums;不对的
                        std::cout << "拷贝构造serviceCondition" <<std::endl;
                }
                //注意，移动构造前后依然是两个对象，只不过如果原来的对象里有堆内存的空间，使用移动构造，可以使用指针赋值的方式转移堆内存空间，前后依然要创建两个对象
                //所以类对象中都是栈上的话，没有区别
                serviceCondition(serviceCondition&& obj){   //如果不提供这个，那么std::move没用，依然会调用拷贝构造，如果提供了，需要提供operator=，因为如果提供移动，那么operator=会被标记为delete
                        //m_nums = obj.m_nums;
                        std::cout << "移动构造serviceCondition" <<std::endl;
                }
                serviceCondition& operator=(const serviceCondition& obj){  
                        //m_nums = obj.m_nums;
                        std::cout << "赋值serviceCondition" <<std::endl;
                        return *this;
                }
                ~serviceCondition(){
                        std::cout << "正在析构serviceCondition" << "nums:" << m_clientNums << std::endl;
                }

                //获取初始评价
                int getInitialCondition(const std::multiset<serviceCondition*,ptrCmp>& routerSet){
                        //产生第7为到第14位的随机小数，刚好支持百万级别的（后面8位非常不容易重复）
                        //第15位一直为0，因为为了比较，保险一点，保证有终止点
                        //上面不再采用，采用5位表示10^5保证最大为10000台的服务器工作
                        //应该是2^16次方
                        //怎么确保初始评价唯一，只能一个一个遍历，然后取出低16位比较、
                        auto it = routerSet.begin();
                        int tempCondition = 0;
                        while(it != routerSet.end()){
                                std::random_device rd; // Non-determinstic seed source
                                std::default_random_engine rng {rd()}; // Create random number generator
                                //std::uniform_real_distribution<int> u(0,65535);
                                std::uniform_int_distribution<int> u(0,65535);
                                tempCondition = u(rd);
                                for(;it !=  routerSet.end(); it++){  //这个不能用范围for
                                        if((*it)->m_initalCondition == tempCondition){  //(*it)->m_initalCondition 这个地方一定要注意
                                                it = routerSet.begin();
                                                break;
                                        }
                                }
                        }
                        return tempCondition;
                }
                //设置比较函数（重载<）（这时候应该着重比较真正的因素）
                bool operator<(const serviceCondition& obj){ //排序和查找都要用到
                        return m_condition < obj.m_condition;
                }
                void refreshCondition(){
                        m_condition =  static_cast<int>(m_clientNums * weight["m_clientNums"])<<16;
                        m_condition |= m_initalCondition;
                }
        public:
                //由于之前的测试，这里都采用unsigned int，而不再采用之前的double，可以看之前的测试
                //unsigned int一共有10位，高5位给m_condition，低5位给m_initalCondition，然后两者相加得到排序评价，好像有可能相等
                //按位？反正按位也是平分的，然后产生完移动一半即可
                ServerAddr m_addr;
                unsigned int m_condition;  //（0-128）？（排序评价）（高16位怎么安排都行，低16位给初始评价）
                unsigned short m_initalCondition; //两个字节 （0-65535）
                double readCondition;  //这个是真实地评价
                int m_clientNums;  //当前服务客户端地个数
                std::unordered_map<std::string, float> weight;  //应该从配置文件中读取
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
        //插入测试
        serviceCondition* server1 = new serviceCondition("127.0.0.1", 10000, 1, routerSet);
        serviceCondition* server2 = new serviceCondition("127.0.0.1", 10001, 1, routerSet);
        serviceCondition* server3 = new serviceCondition("127.0.0.1", 10001, 2, routerSet);
        std::cout << "set中key和unordered_map中value为指针测试开始" << std::endl;
        insertSetElem(serverMap, routerSet, server1);
        insertSetElem(serverMap, routerSet, server2);
        insertSetElem(serverMap, routerSet, server3);
        std::cout <<"插入之后"<< std::endl;
        for(auto key : routerSet){
                std::cout <<"routerSet" << "   "<< key->m_clientNums << std::endl;
        }
        for(auto key : serverMap){
                std::cout <<"serverMap" << "   " <<key.first.m_ip << "   " <<  key.first.m_port << std::endl;
        }

        //修改测试
        ServerAddr serverAddr = ServerAddr("127.0.0.1", 10000);
        auto targetServer = serverMap.find(serverAddr);
        if( targetServer != serverMap.end()){
                //有这个元素，删除掉
                //找到routerSet中该元素的原始地址
                serviceCondition* condition = targetServer->second;
                routerSet.find(condition);
                //取出原始地址
                routerSet.erase(condition);
                condition->m_clientNums += 3;
                routerSet.insert(condition);
        }
        std::cout <<"修改之后" <<  std::endl;
        for(auto key : routerSet){
                std::cout <<"routerSet" << "   " <<  key->m_clientNums << std::endl;
        }
        for(auto key : serverMap){
                std::cout <<"serverMap" << "   " <<  key.first.m_ip << "   " <<  key.first.m_port << "  "<<key.second->m_clientNums <<std::endl;
        }
        std::cout << "set中key和unordered_map中value为指针测试结束" << std::endl;
        return 0;
}

