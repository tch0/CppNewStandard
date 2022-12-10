#include <format>
#include <string>
#include <iostream>

int main(int argc, char const *argv[])
{
    std::cout << std::format("{:_^10} : {:*>10} : {:s}", "hello", "world", true) << std::endl;
    std::cout << std::format("{:_^#20.5F}", 1.00000001e3) << std::endl;
    std::cout << std::format("{:_^20.4A}", 1.212342) << std::endl;
    std::cout << std::format("{:_>+#10X}", -123) << std::endl;
    std::cout << std::format("{:_^#10X}", 1234) << std::endl;
    const char *p = "hello";
    std::cout << std::format("{:s}", p) << std::endl;
    std::cout << std::format("{:p}", p) << std::endl;
    return 0;
}
