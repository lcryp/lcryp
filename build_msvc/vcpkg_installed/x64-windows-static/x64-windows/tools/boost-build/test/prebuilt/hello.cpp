#include <a.h>
int main()
{
    #ifdef RELEASE
    release();
    #else
    debug();
    #endif
    return 0;
}