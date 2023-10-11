#include <iomanip>
#include <iostream>
#include <cmath> //必须要包含这个头文件，才会有double的，如果不用，就是c语言的abs，那个只能是整数，输出为0
//而且double要使用fabs
int main()
{
        double epsilon=1e-11;  //必须要为11，10的话就包不住了
        double d1= 0.0000000001;
        double d2= 0.001;
        double d3 = 0.002;
        double d4 = 322.331;
        int num = 3;
        //真实评价范围0.01-10000.01（7位）
        //double的有效位数为15位。因为double有效数字为52，2^52有15位，所以有效范围为15位
        //产生的小数范围为0.001到0.0000000001（10位有效数字）（为啥）（因为是按照100万台服务器设计的，
        //如果位数不够，会有大量重复的元素。不好排序）
        //评价法则，服务性能越好，评价分数越高（刚初始化时为满分，后面7位随机生成，这7位一定要保证必然是不同的）
        //服务一台扣多少分，服务10台扣完（目前）
        //比较法则
        //直接比较即可，所有位数全都比较，由于最后7位不一样，所以都不可能相等,

        //相等法则
        //如果两个变量相减的绝对值小于1e-11，就认为相等，其实我打印了一下，如果两个数完全相等，
        //那么差值为0，一个小数位都没用
        //不过又碰到一个难题，就是不支持浮点数相与，这该咋办，其实是不用管的，我后面的condition一定是在前面的，
        //和后面不可能有关系,但是测试了好几次，发现确实会变化，可以写一个循环，生成真实评价测试一下，看看
        //真实评价测试，大量相减之后会在-10的量级上徘徊，但是突然意识到不应该是7位，第一假如10000台服务器，那么应该是底数
        //为10，也就是4位就够了，而不是所谓的7位，并且意识到完全可以使用int，在算真实评价之后*10000即可，然后想与，得到
        //最终的排序评价，如果想要得到真实评价，除以10000即可，这个就没有精度上的丢失
        //所以采用整数更合理一些，不过还是采用浮点数吧，但是排序的比较和查找的比较不一样，排序的比较是保证大家都不一样，
        //查找的比较是我已经得到原值，不对，find的时候还是按照<进行比较来判断是否相等。。。。。
        //算了采用整数吧，浮点数确实很难把控精度
        double sum = d2+d1;
        std::cout<<"epsilon is: "<< std::setprecision(30) << epsilon<<std::endl;
        std::cout<<"d2 is: "<< std::setprecision(30) << d2<<std::endl;
        std::cout<<"d3 is: "<< std::setprecision(30) << d3<<std::endl;
        std::cout<<"num s: "<< std::setprecision(30) << num <<std::endl;
        std::cout<<"sum   is: "<< std::setprecision(30) <<  sum <<std::endl;
        sum += d3;
        std::cout<<"sum   is: "<< std::setprecision(30) << sum <<std::endl;
        sum = sum - d3 - d2;
        std::cout<<"sum   is: "<< std::setprecision(30) << sum <<std::endl;
        sum += d4;
        sum -= d4;
        std::cout<<"sum   is: "<< std::setprecision(30) << sum <<std::endl;
        if (fabs(d1-sum) <  epsilon){
                std::cout << "Equal!" << std::endl;
        }
        else{
                std::cout << "Not equal!" << std::endl;
        }
        return 0;
}
