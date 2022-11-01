#include <thread>
#include <memory>
int main()
{
    #ifndef _WIN32
    { auto _ = std::thread::hardware_concurrency(); }
    #endif
    { const std::unique_ptr <float> pf {new float {3.14159f}}; }
}
