#pragma once

#ifdef CDATAUTILS_HASHMAP_ASSERTS
#include <assert.h>
#else
#define assert(...) (void)(__VA_ARGS__)
#endif

struct hashmap 
{
    int capacity;

    void* keys;
    void* values;
};

#define CDATAUTILS_HASHMAP_DEFINE_PAIR(key_type, value_type)
#define CDATAUTILS_HASHMAP_DEFINE_PAIR_NAMED(name, key_type, value_type)

CDATAUTILS_HASHMAP_DEFINE_PAIR(int, int);
