#include <iostream>
#include <string>
#include <cstring>
using std::cout;
using std::string;
using std::strlen;
extern const char class_template[];
extern const char usage[];
int main(int ac, char* av[])
{
    if (av[1]) {
        string class_name = av[1];
        string s = class_template;
        string::size_type n;
        while((n = s.find("%class_name%")) != string::npos) {
            s.replace(n, strlen("%class_name%"), class_name);
        }
        std::cout << "Output is:\n";
        std::cout << s << "\n";
        return 0;
    } else {
        std::cout << usage << "\n";
        return 1;
    }
}
