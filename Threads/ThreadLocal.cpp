#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

std::atomic<int> var{10};
thread_local int val = var++;
std::mutex mcout; // for cout

int main(int argc, char const *argv[])
{
    auto f = []() {
        for (int i = 0; i < 10; ++i)
        {
            {
                std::lock_guard<std::mutex> guard(mcout);
                std::cout << "thread " << std::this_thread::get_id() << ": val = " << val++ << std::endl;
            }
            std::this_thread::sleep_for(100ms);
        }
    };
    std::jthread t(f);
    for (int i = 0; i < 10; ++i)
    {
        {
            std::lock_guard<std::mutex> guard(mcout);
            std::cout << "thread " << std::this_thread::get_id() << ": val = " << val++ << std::endl;
        }
        std::this_thread::sleep_for(100ms);
    }
    return 0;
}
