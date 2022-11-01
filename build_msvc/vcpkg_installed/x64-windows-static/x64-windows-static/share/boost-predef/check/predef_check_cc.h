#include <boost/predef.h>
#ifdef CHECK
#   if ((CHECK) == 0)
#       error "FAILED"
#   endif
#endif
int dummy()
{
	static int d = 0;
	return d++;
}
