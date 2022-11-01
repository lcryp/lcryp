#ifndef FRAMES_DWA20011021_H
#define FRAMES_DWA20011021_H
#include "config.h"
#include "lists.h"
#include "modules.h"
#include "object.h"
typedef struct frame FRAME;
void frame_init( FRAME * );
void frame_free( FRAME * );
struct frame
{
    FRAME      * prev;
    FRAME      * prev_user;
    LOL          args[ 1 ];
    module_t   * module;
    OBJECT     * file;
    int          line;
    char const * rulename;
#ifdef JAM_DEBUGGER
    void       * function;
#endif
    inline frame() { frame_init( this ); }
    inline ~frame() { frame_free( this ); }
    inline frame(const frame & other)
        : prev(other.prev)
        , prev_user(other.prev_user)
        , module(other.module)
        , file(other.file)
        , line(other.line)
        , rulename(other.rulename)
        #ifdef JAM_DEBUGGER
        , function(other.function)
        #endif
    {
        lol_init(args);
        for (int32_t a = 0; a < other.args->count; ++a)
        {
            lol_add(args, list_copy(other.args->list[a]));
        }
    }
};
extern FRAME * frame_before_python_call;
#endif
