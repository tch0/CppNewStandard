#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <list>
#include <string>
#include <atomic>

using namespace std::chrono_literals;
namespace chrono = std::chrono;

// a simple implementation of std::this_thread::sleep_for
template<typename Rep, typename Period>
void my_sleep_for(const chrono::duration<Rep, Period>& d)
{
    std::condition_variable timer;
    std::mutex mtx;
    auto t0 = chrono::steady_clock::now();
    std::unique_lock<std::mutex> lck(mtx);
    timer.wait_for(lck, d);
    auto t1 = chrono::steady_clock::now();
    std::cout << chrono::duration_cast<chrono::milliseconds>(t1-t0) << " passed" << std::endl;
}

// reimplement consumer/producer
template<typename T>
class SyncQueue
{
public:
    void put(const T& val)
    {
        std::lock_guard lck(mtx);
        q.push_back(val);
        cond.notify_one();
    }
    void put(T&& val)
    {
        std::lock_guard lck(mtx);
        q.push_back(std::move(val));
        cond.notify_one();
    }
    bool get(T& val) // return success or not
    {
        std::unique_lock lck(mtx);
        bool res = cond.wait_for(lck, 1ms, [this]() { return !q.empty(); });
        if (!res)
        {
            return false;
        }
        val = q.front();
        q.pop_front();
        return true;
    }
private:
    std::mutex mtx;
    std::condition_variable cond;
    std::list<T> q;
};

using Message = std::string;
SyncQueue<Message> mq;
std::atomic_flag end_of_produce { ATOMIC_FLAG_INIT };

std::mutex mcout; // for cout

void producer()
{
    for (int i = 0; i < 200; ++i)
    {
        {
            std::lock_guard lck(mcout);
            std::cout << "producer("<< std::this_thread::get_id() << "): " << i << std::endl;
        }
        mq.put(std::to_string(i));
        // std::this_thread::sleep_for(3ms);
    }
    end_of_produce.test_and_set();
}

void consumer()
{
    while (true)
    {
        Message message;
        bool res = mq.get(message);
        // end of consuming
        if (!res && end_of_produce.test())
        {
            std::lock_guard lck(mcout);
            std::cout << "\t\t\tend of consuming(" << std::this_thread::get_id() << ")" << std::endl;
            return;
        }
        // consuming
        if (res)
        {
            std::lock_guard lck(mcout);
            std::cout << "\t\t\tconsumer("<< std::this_thread::get_id() << "): " << message << std::endl;
        }
    }
}

int main(int argc, char const *argv[])
{
    my_sleep_for(100ms);

    std::jthread p(producer);
    std::jthread c1(consumer);
    std::jthread c2(consumer);
    std::jthread c3(consumer);
    std::jthread c4(consumer);
    return 0;
}
