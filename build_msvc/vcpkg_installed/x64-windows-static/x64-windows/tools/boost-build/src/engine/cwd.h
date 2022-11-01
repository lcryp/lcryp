#ifndef CWD_H
#define CWD_H
#include "config.h"
#include "object.h"
#include <string>
OBJECT * cwd( void );
namespace b2
{
    const std::string & cwd_str();
}
void cwd_init( void );
void cwd_done( void );
#endif
