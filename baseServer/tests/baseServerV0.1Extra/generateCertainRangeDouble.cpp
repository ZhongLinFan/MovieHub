#include <iostream>
#include <random>

//产生的小数范围为0.001到0.0000000001（10位有效数字）
//产生0.01-10000.01的随机数
int main()
{
  std::random_device rd; // Non-determinstic seed source
      std::default_random_engine rng {rd()}; // Create random number generator
          std::uniform_real_distribution<double> u(0.0000000001,0.001);
              std::cout << u(rd) <<std::endl;
                 return 0;
                 }
