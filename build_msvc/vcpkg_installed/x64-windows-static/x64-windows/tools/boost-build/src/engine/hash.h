#ifndef BOOST_JAM_HASH_H
#define BOOST_JAM_HASH_H
#include "config.h"
#include "object.h"
typedef struct hashdata HASHDATA;
struct hash * hashinit( int32_t datalen, char const * name );
void hash_free( struct hash * );
void hashdone( struct hash * );
typedef void (* hashenumerate_f)( void *, void * );
void hashenumerate( struct hash *, void (* f)( void *, void * ), void * data );
template <typename T, typename D>
void hash_enumerate( struct hash * h, void (* f)(T *, D *), D * data)
{
    hashenumerate(h, reinterpret_cast<hashenumerate_f>(f), data);
}
template <typename T, typename D>
void hash_enumerate( struct hash * h, void (* f)(T *, D *))
{
    hashenumerate(h, reinterpret_cast<hashenumerate_f>(f), nullptr);
}
HASHDATA * hash_insert( struct hash *, OBJECT * key, int32_t * found );
HASHDATA * hash_find( struct hash *, OBJECT * key );
struct hashstats {
    int count;
    int num_items;
    int tab_size;
    int item_size;
    int sets;
    int num_hashes;
};
void hashstats_init( struct hashstats * stats );
void hashstats_add( struct hashstats * stats, struct hash * );
void hashstats_print( struct hashstats * stats, char const * name );
#endif
