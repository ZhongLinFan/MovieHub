#include <iostream>
#include <set>
#include <unordered_map>

struct serviceCondition{
        public:
                int nums;
                serviceCondition(){
                        std::cout << "无参构造serviceCondition" <<std::endl;
                }
                serviceCondition(int i){
                        std::cout << "正在生成serviceCondition" <<std::endl;
                        nums = i;
                }
                serviceCondition(const serviceCondition& obj){  //必须要加const，否则无法转换会报错
                        nums = obj.nums;
                        //obj.nums = nums;不对的
                                std::cout << "拷贝构造serviceCondition" <<std::endl;
                }
                ~serviceCondition(){
                        std::cout << "正在析构serviceCondition" << "nums:" << nums << std::endl;
                }
};

class cmp{
        public:
                bool operator()(const serviceCondition& left, const serviceCondition& right){
                        return left.nums < right.nums;
                }
};

int main()
{
        std::set<serviceCondition*> routerSet;
        std::unordered_map<int, serviceCondition*>hostMap;
        serviceCondition* i = new serviceCondition(1);
        std::cout << "key为指针测试" << std::endl;
        routerSet.insert(i);

        hostMap[10010] = i;
        delete i;
        for(auto key : routerSet){
                std::cout <<"start" <<  key->nums << "end" << std::endl;
        }
        std::cout << "指针测试结束" << std::endl;

        std::set<serviceCondition, cmp> routerSet1;
        //std::unordered_map<int, serviceCondition>hostMap1;
        serviceCondition* j = new serviceCondition(2);
        std::cout << "key为对象测试" << std::endl;
        routerSet1.insert(*j);

        //hostMap1[10010] = *j;
        delete j;

        for(const auto& key : routerSet1){
                std::cout <<"start" <<  key.nums << "end" << std::endl;
        }

        std::cout << "对象测试结束" << std::endl;
}

