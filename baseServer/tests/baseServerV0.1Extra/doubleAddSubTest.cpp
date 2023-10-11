#include <iostream>
#include <random>
#include <iomanip>

//产生的小数范围为0.001到0.0000000001（10位有效数字）
//产生0.01-10000.01的随机数

int main()
{
        std::random_device rd; // Non-determinstic seed source
        std::default_random_engine rng {rd()}; // Create random number generator
        std::uniform_real_distribution<double> u0(0.0000000001,0.001);
        std::uniform_real_distribution<double> u1(0.01,10000.01);
        std::uniform_real_distribution<double> u2(-10000.01,-0.01);
        double d0 = u0(rd);
        double condition = 10000.01 ;
        double start = condition + d0;
        int notEqualTimes = 0;
        for(int i = 0; i < 10000000; i++){
                double freshValue1 = u1(rd);
                double freshValue2 = u2(rd);
                start = start + freshValue1 + freshValue2;
                start = start - freshValue1 - freshValue2;
                std::cout<<"  "<< std::setprecision(30) << start <<std::endl;
                std::cout<<"  "<< std::setprecision(30) << fabs(start-condition - d0) <<std::endl;
                if (fabs(start-condition - d0) < 1e-10){
                        std::cout << "Equal!" << std::endl;
                }
                else{
                        notEqualTimes++;
                        std::cout << "Not equal!" << std::endl;

                }
        }
        std::cout << notEqualTimes <<std::endl;
        std::cout << std::setprecision(30) << d0 << std::endl;
        std::cout << std::setprecision(30) << start-start << std::endl;
        return 0;
}
