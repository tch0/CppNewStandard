#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <chrono>

using namespace std::chrono_literals;

// 线程安全的同步队列
template<typename T>
class SyncQueue
{
public:
    void put(T t)
    {
        std::lock_guard<std::mutex> guard(m);
        q.push(t);
    }
    T get()
    {
        std::lock_guard<std::mutex> guard(m);
        T res = q.front();
        q.pop();
        return res;
    }
    bool empty()
    {
        std::lock_guard<std::mutex> guard(m);
        return q.empty();
    }
private:
    std::mutex m; // for queue
    std::queue<T> q;
};

using Message = std::string;

std::mutex mcout; // for cout, avoid interleaved output

// 生产者消费者模型
struct Consumer
{
    SyncQueue<Message>& q;
    Consumer(SyncQueue<Message>& _q) : q(_q) {}
    void operator()()
    {
        while (true)
        {
            if (!q.empty())
            {
                std::lock_guard<std::mutex> guard(mcout);
                Message message = q.get();
                std::cout << "\t\t\t" << "consumer: " << message << std::endl;
            }
        }
    }
};

struct Producer
{
    SyncQueue<Message>& q;
    Producer(SyncQueue<Message>& _q) : q(_q) {}
    void operator()()
    {
        while (count < 100)
        {
            {
                std::lock_guard<std::mutex> guard(mcout);
                Message message = std::to_string(count++);
                q.put(message);
                std::cout << "producer: " << message << std::endl;
            }
            std::this_thread::sleep_for(1ms);
        }
        return;
    }
    inline static int count = 0;
};

void my_terminate()
{
    std::cout << "terminate" << std::endl;
}

int main(int argc, char const *argv[])
{
    std::set_terminate(my_terminate);
    SyncQueue<Message> mq;
    Consumer c(mq);
    Producer p(mq);
    std::thread tc(c);
    std::thread tp(p);
    tc.detach(); // consumer end by ending of main thread
    tp.join(); // producer end by itself
    std::cout << "end of main thread" << std::endl;
    return 0;
}
