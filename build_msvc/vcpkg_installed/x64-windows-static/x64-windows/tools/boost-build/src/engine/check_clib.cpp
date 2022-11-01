#include <string.h>
int check_clib()
{
    { auto _ = strdup("-"); }
	return 0;
}
