# include "jam.h"
# include "option.h"
int getoptions( int argc, char * * argv, const char * opts, bjam_option * optv )
{
    int i;
    int optc = N_OPTS;
    memset( (char *)optv, '\0', sizeof( *optv ) * N_OPTS );
    for ( i = 0; i < argc; ++i )
    {
        char *arg;
        if ( ( argv[ i ][ 0 ] != '-' ) ||
            ( ( argv[ i ][ 1 ] != '-' ) && !isalpha( argv[ i ][ 1 ] ) ) )
            continue;
        if ( !optc-- )
        {
            printf( "too many options (%d max)\n", N_OPTS );
            return -1;
        }
        for ( arg = &argv[ i ][ 1 ]; *arg; ++arg )
        {
            const char * f;
            for ( f = opts; *f; ++f )
                if ( *f == *arg )
                    break;
            if ( !*f )
            {
                printf( "Invalid option: -%c\n", *arg );
                return -1;
            }
            optv->flag = *f;
            if ( f[ 1 ] != ':' )
            {
                optv++->val = (char *)"true";
            }
            else if ( arg[ 1 ] )
            {
                optv++->val = &arg[1];
                break;
            }
            else if ( ++i < argc )
            {
                optv++->val = argv[ i ];
                break;
            }
            else
            {
                printf( "option: -%c needs argument\n", *f );
                return -1;
            }
        }
    }
    return i;
}
char * getoptval( bjam_option * optv, char opt, int subopt )
{
    int i;
    for ( i = 0; i < N_OPTS; ++i, ++optv )
        if ( ( optv->flag == opt ) && !subopt-- )
            return optv->val;
    return 0;
}
