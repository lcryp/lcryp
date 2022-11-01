#ifndef BOOST_PREDEF_WORKAROUND_H
#define BOOST_PREDEF_WORKAROUND_H
#ifdef BOOST_STRICT_CONFIG
#   define BOOST_PREDEF_WORKAROUND(symbol, comp, major, minor, patch) (0)
#else
#   include <boost/predef/version_number.h>
#   define BOOST_PREDEF_WORKAROUND(symbol, comp, major, minor, patch) \
        ( (symbol) != (0) ) && \
        ( (symbol) comp (BOOST_VERSION_NUMBER( (major) , (minor) , (patch) )) )
#endif
#ifdef BOOST_STRICT_CONFIG
#   define BOOST_PREDEF_TESTED_AT(symbol, major, minor, patch) (0)
#else
#   ifdef BOOST_DETECT_OUTDATED_WORKAROUNDS
#       define BOOST_PREDEF_TESTED_AT(symbol, major, minor, patch) ( \
            BOOST_PREDEF_WORKAROUND(symbol, <=, major, minor, patch) \
            ? 1 \
            : (1%0) )
#   else
#       define BOOST_PREDEF_TESTED_AT(symbol, major, minor, patch) \
            ( (symbol) >= BOOST_VERSION_NUMBER_AVAILABLE )
#   endif
#endif
#endif
