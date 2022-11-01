#include <iostream>
int main()
{
    char* c = nullptr;
    std::cout << "Hello sanitizers\n " << *c;
}
