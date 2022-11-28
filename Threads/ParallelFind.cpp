#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cassert>

using namespace std::chrono_literals;

template<typename T, typename Predicate>
auto pfind(const std::vector<T>& vec, Predicate p)
{
    const int grain = 50000; // the grain-size of parallel, need to profile to find a best grain-size
    assert(vec.size() % grain == 0);
    using Iterator = typename std::vector<T>::const_iterator;

    std::vector<std::future<std::pair<Iterator, bool>>> res;
    auto my_find_if = [](auto beg, auto end, auto pred) {
        auto res = std::find_if(beg, end, pred);
        return std::make_pair(res, res != end);
    };
    for (int i = 0; i < vec.size(); i+=grain)
    {
        res.push_back(std::async(std::launch::async, my_find_if, vec.begin() + i, vec.begin() + i + grain, p));
    }
    for (int i = 0; i < res.size(); ++i)
    {
        auto p = res[i].get();
        if (p.second)
        {
            return p.first;
        }
    }
    return vec.end();
}

int main(int argc, char const *argv[])
{
    std::vector<int> vec(1000000);
    std::iota(vec.begin(), vec.end(), 0);
    {
        auto t0 = std::chrono::steady_clock::now();
        auto res = pfind(vec, [](int val) { return val == 997830; });
        std::cout << *res << std::endl;
        auto t1 = std::chrono::steady_clock::now();
        std::cout << "parallel find: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0) << std::endl;
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        auto res = std::find_if(vec.begin(), vec.end(), [](int val) { return val == 997830; });
        std::cout << *res << std::endl;
        auto t1 = std::chrono::steady_clock::now();
        std::cout << "normal find: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0) << std::endl;
    }
    return 0;
}
