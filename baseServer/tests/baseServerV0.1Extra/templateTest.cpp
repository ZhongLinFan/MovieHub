#include <iostream>
#include <typeinfo>
#include <initializer_list>

template<class T>
void printTemplateArgs(std::string name, std::initializer_list<int> list){
        std::cout << *(list.begin()) <<std::endl;
}

template<class Arg1,class Arg, class... Args>
void printTemplateArgs(std::string name, std::initializer_list<int> list){
        std::cout << typeid(Arg).name() << std::endl;
        int i = 0;
        printTemplateArgs<std::string, Arg, Args...>(name, list);
       // int array[] = {(printTemplateArgs<Args>(i++, list),0)...};
}

int main()
{
        std::initializer_list<int> list = {1,2,3,4,5,6};
        printTemplateArgs<std::string, int, double, float, bool, char, short>("test",list);
}
