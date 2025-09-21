#include"common.hpp"
#include<chrono>
#include<ctime>
#include<iomanip>
namespace stonedb
{
    void log(const std::string& msg)
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] " << msg << std::endl;
    }
    void logError(const std::string& msg)
    {
    std::cerr << "ERROR: " << msg << std::endl;
}
}
