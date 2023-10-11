#include <iostream>
#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <condition_variable>
#include <chrono>
using namespace std;

class SyncQueue
{
        public:
                SyncQueue(int maxSize) : m_maxSize(maxSize) {}

                void put(const int& x)
                {
                    while(1){
                    {
                        unique_lock<mutex> locker(m_mutex);
                        // 判断任务队列是不是已经满了
                        while (m_queue.size() == m_maxSize)
                        {
                                std::cout <<"线程id："<< std::this_thread::get_id() << "任务队列已满" << std::endl;
                                // 阻塞线程
                                m_notFull.wait(locker);
                        }
                        // 将任务放入到任务队列中
                       // while(m_queue.size() < m_maxSize){
                            m_queue.push_back(x);
                            cout <<"线程id："<< std::this_thread::get_id() << " "  << x << " 被生产" << endl; 
                            //通知消费者去消费，注意如果消费者阻塞在空队列，采取这种方式去通知，会导致生产者生产满，阻塞在消费者的线程才会被唤醒，这个和我的需求不太一致，我的需求是如果消费者被阻塞在空的队列上，那这里生产完一个，应该立马被唤醒
                    }//加上这个小括号，至于为啥加就是因为上面这个描述，原因是啥https://www.jianshu.com/p/3721ed62742d，这里也介绍了，https://blog.csdn.net/zgaoq/article/details/109644461，准确的说是无效唤醒，cpp官网就说不建议带锁唤醒，说这是一种悲观的做法，第二个链接中有相关的连接。加上这个小括号，可以解决及时让消费者消费的问题，但是通过日志打印，可以发现并不够及时，也就是生产者还是极大概率会继续生产满(即使内部不加while,我看网上说唤醒可能只是处于就绪状态，剩下的就靠抢了，我打印日志也发现，消费者线程可以抢到，但是概率极低，可能是消费者线程的优先级比较低吧。。。。)，消费者才会得到访问权限。那就让当前线程主动让出一会.这样让出一会可以基本解决我的需求，但是有一点不优雅的是，通过sleep_for让出后，会导致生产线程不断地切换去生产，这样会导致多余的切换，我只需要一个切换其实就够了 
                        m_notEmpty.notify_one();
                        std::this_thread::sleep_for(std::chrono::microseconds(1));//通过上面的讨论和下面这一行的结论，决定还是采用这一行来解决
                        //std::this_thread::yield();//但是发现这并没有效果。。。。。也就是还是会生产完再执行，反而上面的microseconds有点作用,发现https://noerror.net/zh/cpp/thread/yield.html，发现如果当前优先级队列中如果没有相同的线程，那么这个函数是无用的。。。。
                       // }
                    }
                }

                int take()
                {
                    while(1){
                    {
                        unique_lock<mutex> locker(m_mutex);
                        while (m_queue.empty())
                        {
                                std::cout <<"线程id："<< std::this_thread::get_id() << "任务队列已空，请耐心等待。。。" << std::endl;
                                m_notEmpty.wait(locker);
                        }
                        // 从任务队列中取出任务(消费)
                        int x = m_queue.front();
                        m_queue.pop_front();
                        // 通知生产者去生产
                        std::cout <<"线程id："<< std::this_thread::get_id() << " ";
                        if(m_queue.size() <= 0){
                            std::cout << "生产者赶紧起来干活" << " ";
                            m_notFull.notify_one();
                        }
                        std::cout  << x << "被消费" << std::endl;
                        //模拟缓慢读取帧
                    }//注意为啥这里加个括号，如果不加括号，那就代表带着锁休眠，也就是其他线程进入不了。。。。。所以休眠一定要释放锁
                        std::this_thread::sleep_for(std::chrono::milliseconds(24));
                       // return x;如果有返回值，那么while没作用，那么当前线程执行完一次take就结束了
                    }
                }

                bool empty()
                {
                        lock_guard<mutex> locker(m_mutex);
                        return m_queue.empty();
                }

                bool full()
                {
                        lock_guard<mutex> locker(m_mutex);
                        return m_queue.size() == m_maxSize;
                }

                int size()
                {
                        lock_guard<mutex> locker(m_mutex);
                        return m_queue.size();
                }

        private:
                list<int> m_queue;     // 存储队列数据
                mutex m_mutex;         // 互斥锁
                condition_variable m_notEmpty;   // 不为空的条件变量
                condition_variable m_notFull;    // 没有满的条件变量
                int m_maxSize;         // 任务队列的最大任务个数
};

int main()
{
        SyncQueue taskQ(11);
        auto produce = bind(&SyncQueue::put, &taskQ, placeholders::_1);
        auto consume = bind(&SyncQueue::take, &taskQ);
        thread t1[3];
        thread t2[3];
        for (int i = 0; i < 3; ++i)
        {
                t1[i] = thread(produce, i+100);
                t2[i] = thread(consume);
        }

        for (int i = 0; i < 3; ++i)
        {
                t1[i].join();
                t2[i].join();
        }

        return 0;
}


