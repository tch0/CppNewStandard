#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

std::atomic_flag killFlag { ATOMIC_FLAG_INIT };

int main(int argc, char const *argv[])
{
    auto f = []() {
        while (true)
        {
            if (killFlag.test())
            {
                std::cout << "receive kill signal : kill thread " << std::this_thread::get_id() << std::endl;
                break;
            }
            std::this_thread::sleep_for(100ms);
        }
    };
    std::jthread t(f); // auto join when destruction
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "main thread running" << std::endl;
        std::this_thread::sleep_for(50ms);
    }
    killFlag.test_and_set();
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "main thread ending" << std::endl;
        std::this_thread::sleep_for(50ms);
    }
    return 0;
}
