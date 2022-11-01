#include "config.h"
typedef struct bjam_option
{
    char flag;
    char * val;
} bjam_option;
#define N_OPTS 256
int    getoptions( int argc, char * * argv, const char * opts, bjam_option * optv );
char * getoptval( bjam_option * optv, char opt, int subopt );
