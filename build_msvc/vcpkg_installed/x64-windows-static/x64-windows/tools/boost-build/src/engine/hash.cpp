#include "jam.h"
#include "hash.h"
#include "compile.h"
#include "output.h"
#include <assert.h>
typedef struct item ITEM;
struct item
{
    ITEM * next;
};
#define MAX_LISTS 32
struct hash
{
    struct
    {
        int32_t nel;
        ITEM * * base;
    } tab;
    int32_t bloat;
    int32_t inel;
    struct
    {
        int32_t more;
        ITEM * free;
        char * next;
        int32_t size;
        int32_t nel;
        int32_t list;
        struct
        {
            int32_t nel;
            char * base;
        } lists[ MAX_LISTS ];
    } items;
    char const * name;
};
static void hashrehash( struct hash * );
static void hashstat( struct hash * );
static uint32_t hash_keyval( OBJECT * key )
{
    return object_hash( key );
}
#define hash_bucket(hp, keyval) ((hp)->tab.base + ((keyval) % (hp)->tab.nel))
#define hash_data_key(data) (*(OBJECT * *)(data))
#define hash_item_data(item) ((HASHDATA *)((char *)item + sizeof(ITEM)))
#define hash_item_key(item) (hash_data_key(hash_item_data(item)))
#define ALIGNED(x) ((x + sizeof(ITEM) - 1) & ~(sizeof(ITEM) - 1))
struct hash * hashinit( int32_t datalen, char const * name )
{
    struct hash * hp = (struct hash *)BJAM_MALLOC( sizeof( *hp ) );
    hp->bloat = 3;
    hp->tab.nel = 0;
    hp->tab.base = 0;
    hp->items.more = 0;
    hp->items.free = 0;
    hp->items.size = sizeof( ITEM ) + ALIGNED( datalen );
    hp->items.list = -1;
    hp->items.nel = 0;
    hp->inel = 11;
    hp->name = name;
    return hp;
}
static ITEM * hash_search( struct hash * hp, uint32_t keyval,
    OBJECT * keydata, ITEM * * previous )
{
    ITEM * i = *hash_bucket( hp, keyval );
    ITEM * p = 0;
    for ( ; i; i = i->next )
    {
        if ( object_equal( hash_item_key( i ), keydata ) )
        {
            if ( previous )
                *previous = p;
            return i;
        }
        p = i;
    }
    return 0;
}
HASHDATA * hash_insert( struct hash * hp, OBJECT * key, int32_t * found )
{
    ITEM * i;
    uint32_t keyval = hash_keyval( key );
    #ifdef HASH_DEBUG_PROFILE
    profile_frame prof[ 1 ];
    if ( DEBUG_PROFILE )
        profile_enter( 0, prof );
    #endif
    if ( !hp->items.more )
        hashrehash( hp );
    i = hash_search( hp, keyval, key, 0 );
    if ( i )
        *found = 1;
    else
    {
        ITEM * * base = hash_bucket( hp, keyval );
        if ( hp->items.free )
        {
            i = hp->items.free;
            hp->items.free = i->next;
            assert( !hash_item_key( i ) );
        }
        else
        {
            i = (ITEM *)hp->items.next;
            hp->items.next += hp->items.size;
        }
        --hp->items.more;
        i->next = *base;
        *base = i;
        *found = 0;
    }
    #ifdef HASH_DEBUG_PROFILE
    if ( DEBUG_PROFILE )
        profile_exit( prof );
    #endif
    return hash_item_data( i );
}
HASHDATA * hash_find( struct hash * hp, OBJECT * key )
{
    ITEM * i;
    uint32_t keyval = hash_keyval( key );
    #ifdef HASH_DEBUG_PROFILE
    profile_frame prof[ 1 ];
    if ( DEBUG_PROFILE )
        profile_enter( 0, prof );
    #endif
    if ( !hp->items.nel )
    {
        #ifdef HASH_DEBUG_PROFILE
        if ( DEBUG_PROFILE )
            profile_exit( prof );
        #endif
        return 0;
    }
    i = hash_search( hp, keyval, key, 0 );
    #ifdef HASH_DEBUG_PROFILE
    if ( DEBUG_PROFILE )
        profile_exit( prof );
    #endif
    return i ? hash_item_data( i ) : 0;
}
static void hashrehash( struct hash * hp )
{
    int32_t i = ++hp->items.list;
    hp->items.more = i ? 2 * hp->items.nel : hp->inel;
    hp->items.next = (char *)BJAM_MALLOC( hp->items.more * hp->items.size );
    hp->items.free = 0;
    hp->items.lists[ i ].nel = hp->items.more;
    hp->items.lists[ i ].base = hp->items.next;
    hp->items.nel += hp->items.more;
    if ( hp->tab.base )
        BJAM_FREE( (char *)hp->tab.base );
    hp->tab.nel = hp->items.nel * hp->bloat;
    hp->tab.base = (ITEM * *)BJAM_MALLOC( hp->tab.nel * sizeof( ITEM * ) );
    memset( (char *)hp->tab.base, '\0', hp->tab.nel * sizeof( ITEM * ) );
    for ( i = 0; i < hp->items.list; ++i )
    {
        int32_t nel = hp->items.lists[ i ].nel;
        char * next = hp->items.lists[ i ].base;
        for ( ; nel--; next += hp->items.size )
        {
            ITEM * i = (ITEM *)next;
            ITEM * * ip = hp->tab.base + object_hash( hash_item_key( i ) ) %
                hp->tab.nel;
            assert( hash_item_key( i ) );
            i->next = *ip;
            *ip = i;
        }
    }
}
void hashenumerate( struct hash * hp, void (* f)( void *, void * ), void * data
    )
{
    int32_t i;
    for ( i = 0; i <= hp->items.list; ++i )
    {
        char * next = hp->items.lists[ i ].base;
        int32_t nel = hp->items.lists[ i ].nel;
        if ( i == hp->items.list )
            nel -= hp->items.more;
        for ( ; nel--; next += hp->items.size )
        {
            ITEM * const i = (ITEM *)next;
            if ( hash_item_key( i ) != 0 )
                f( hash_item_data( i ), data );
        }
    }
}
void hash_free( struct hash * hp )
{
    int32_t i;
    if ( !hp )
        return;
    if ( hp->tab.base )
        BJAM_FREE( (char *)hp->tab.base );
    for ( i = 0; i <= hp->items.list; ++i )
        BJAM_FREE( hp->items.lists[ i ].base );
    BJAM_FREE( (char *)hp );
}
static void hashstat( struct hash * hp )
{
    struct hashstats stats[ 1 ];
    hashstats_init( stats );
    hashstats_add( stats, hp );
    hashstats_print( stats, hp->name );
}
void hashstats_init( struct hashstats * stats )
{
    stats->count = 0;
    stats->num_items = 0;
    stats->tab_size = 0;
    stats->item_size = 0;
    stats->sets = 0;
    stats->num_hashes = 0;
}
void hashstats_add( struct hashstats * stats, struct hash * hp )
{
    if ( hp )
    {
        ITEM * * tab = hp->tab.base;
        int nel = hp->tab.nel;
        int count = 0;
        int sets = 0;
        int i;
        for ( i = 0; i < nel; ++i )
        {
            ITEM * item;
            int here = 0;
            for ( item = tab[ i ]; item; item = item->next )
                ++here;
            count += here;
            if ( here > 0 )
                ++sets;
        }
        stats->count += count;
        stats->sets += sets;
        stats->num_items += hp->items.nel;
        stats->tab_size += hp->tab.nel;
        stats->item_size = hp->items.size;
        ++stats->num_hashes;
    }
}
void hashstats_print( struct hashstats * stats, char const * name )
{
    out_printf( "%s table: %d+%d+%d (%dK+%luK+%luK) items+table+hash, %f density\n",
        name,
        stats->count,
        stats->num_items,
        stats->tab_size,
        stats->num_items * stats->item_size / 1024,
        (long unsigned)stats->tab_size * sizeof( ITEM * * ) / 1024,
        (long unsigned)stats->num_hashes * sizeof( struct hash ) / 1024,
        (float)stats->count / (float)stats->sets );
}
void hashdone( struct hash * hp )
{
    if ( !hp )
        return;
    if ( DEBUG_MEM || DEBUG_PROFILE )
        hashstat( hp );
    hash_free( hp );
}
