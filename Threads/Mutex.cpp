#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std::chrono_literals;

// 同时获取这两个锁才能使用cout
std::recursive_mutex mcout1;
std::recursive_mutex mcout2;

int main(int argc, char const *argv[])
{
    // just for representation
    auto f1 = []() {
        std::this_thread::sleep_for(50ms);
        std::unique_lock<std::recursive_mutex> ul1(mcout1, std::defer_lock); // do not acquire mcout1 now
        std::unique_lock<std::recursive_mutex> ul2(mcout2, std::defer_lock); // do not acquire mcout1 now
        std::scoped_lock sl(ul1, ul2); // acquire two locks now, std::lock is also OK here, dtor of ul1 and ul2 will release mcout1 and mcout2.
        std::cout << "end of thread t1" << std::endl;
    };
    auto f2 = []() {
        std::this_thread::sleep_for(50ms);
        std::unique_lock<std::recursive_mutex> ul1(mcout1, std::defer_lock); // do not acquire mcout1 now
        std::unique_lock<std::recursive_mutex> ul2(mcout2, std::defer_lock); // do not acquire mcout1 now
        std::lock(ul1, ul2); // dtor of std::unique_lock will unlock all locks.
        std::cout << "end of thread t2" << std::endl;
    };
    auto f3 = []() {
        std::this_thread::sleep_for(50ms);
        std::lock(mcout1, mcout2);
        std::unique_lock<std::recursive_mutex> ul1(mcout1, std::adopt_lock); // assume mcout1 is locked
        std::unique_lock<std::recursive_mutex> ul2(mcout2, std::adopt_lock); // assume mcout2 is locked
        std::cout << "end of thread t3" << std::endl;
    };
    // the most primitive way
    auto f4 = []() {
        std::this_thread::sleep_for(50ms);
        std::lock(mcout1, mcout2);
        std::cout << "end of thread t4" << std::endl;
        mcout1.unlock();
        mcout2.unlock();
    };
    std::jthread t1(f1);
    std::jthread t2(f2);
    std::jthread t3(f3);
    std::jthread t4(f4);
    std::this_thread::sleep_for(100ms);
    {
        // the best way.
        std::scoped_lock sl(mcout1, mcout2);
        std::cout << "end of main thread" << std::endl;
    }
    return 0;
}
