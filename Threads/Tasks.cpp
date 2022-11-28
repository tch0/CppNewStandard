#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <vector>

using namespace std::chrono_literals;

std::mutex mcout; // for cout

void f1(const std::vector<double>& vec, std::promise<double> pro, const std::string& s)
{
    double sum{};
    for (auto v : vec)
    {
        sum += v;
        std::this_thread::sleep_for(10ms);
    }
    {
        std::lock_guard lck(mcout);
        std::cout << "f1 thread " << std::this_thread::get_id() << " end : " << s << std::endl;
    }
    pro.set_value(sum);
};

double f2(const std::vector<double>& vec, const std::string& s)
{
    double sum{};
    for (auto v : vec)
    {
        sum += v;
        std::this_thread::sleep_for(10ms);
    }
    {
        std::lock_guard lck(mcout);
        std::cout << "f2 thread " << std::this_thread::get_id() << " end : " << s << std::endl;
    }
    return sum;
}

int main(int argc, char const *argv[])
{
    std::vector<double> vec(100);
    for (int i = 0; i < vec.size(); ++i)
    {
        vec[i] = i+1;
    }
    std::cout << "this thread : " << std::this_thread::get_id() << std::endl;

    // typical use of std::promise without a packaged_task
    std::promise<double> pro1;
    std::future<double> fut1 = pro1.get_future();
    std::jthread t(f1, vec, std::move(pro1), "promise with thread");

    // use std::packaged_task
    std::packaged_task<double(const std::vector<double>&, const std::string&)> task1(f2), task2(f2);
    task1(vec, "task1");
    task2(vec, "task2");
    std::future<double> fut2 = task1.get_future();
    std::future<double> fut3 = task2.get_future();

    // use std::async
    std::future<double> fut4 = std::async(f2, vec, "std::async");
    // use std::async with std::launch::deferred
    std::future<double> fut5 = std::async(std::launch::deferred, f2, vec, "std::async with deferred");
    // use std::async with std::launch::async
    std::future<double> fut6 = std::async(std::launch::async, f2, vec, "std::async with async");

    // results:
    std::cout << fut1.get() << std::endl;
    std::cout << fut2.get() << std::endl; // std::packaged_task run on main thread, why?
    std::cout << fut3.get() << std::endl; // std::packaged_task run on main thread, why?
    std::cout << fut4.get() << std::endl;
    std::cout << fut5.get() << std::endl;
    std::cout << fut6.get() << std::endl; // run on main thread as expected.
    return 0;
}
