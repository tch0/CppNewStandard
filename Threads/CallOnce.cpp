#include <iostream>
#include <mutex>
#include <chrono>

using namespace std::chrono_literals;

class Y
{
public:
    Y()
    {
        std::cout << "default ctor of Y" << std::endl;
    }
    void set(double value)
    {
        std::cout << "set to " << value << std::endl;
        val = value;
    }
    double get()
    {
        return val;
    }
    double val;
};


class X
{
private:
    inline static std::once_flag static_flag;
    inline static Y static_data_for_class_X;
    // the real initialization
    static void init()
    {
        static_data_for_class_X.set(100);
    }
public:
    X()
    {
        call_once(static_flag, init);
    }
};

int main(int argc, char const *argv[])
{
    X();
    X();
    return 0;
}
