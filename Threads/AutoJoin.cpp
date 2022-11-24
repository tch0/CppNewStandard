#include <iostream>
#include <thread>

struct guarded_thread : std::thread
{
    using std::thread::thread;
    ~guarded_thread()
    {
        if (this->joinable())
        {
            this->join();
        }
    }
};

int main(int argc, char const *argv[])
{
    auto f = []() { std::cout << "hello" << std::endl; };
    guarded_thread t(f);
    return 0;
}
