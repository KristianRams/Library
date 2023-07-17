#pragma once

#include "Types.h"
#include "Hash.h"

#include <assert.h>
#include <string.h> // memset
#include <stdlib.h> // calloc

/**

   The structure of Hash_States was primarily inspired by nothings's std_ds.h hash table.

   This is a Hash_Table implemention using Linear Probing. If you don't provide a hash function, the default
   one used is murmur32 hash function and the default table size is 32 slots.

   We use a canonical DELETED sentinel for removed hashes. The other hash states are implemented similar to how
   stb_ds hash hash uses them.

   We pack each entry into an 'Entry' struct for cache reasons so we will have at most 1 cache miss as there is
   a low probability that we will have a collision and therefore will not need to probe outside the cache line.

   Alternatively, using a linked-list for collisions results in multiple cache misses as for each link is a memory access to some random
   region in memory.

**/

enum HASH_STATE : u8 {
    VACANT  = 0,
    DELETED = 1,
    VALID   = 2,
};

inline u32 next_power_of_two(u32 x) {
    assert(x != 0);
    int p = 1;
    while (x > p) { p += p; }

    return p;
}

template <typename Key_Type, typename Value_Type>
struct Hash_Table {
    s32 table_size; // The total size of the table. This should be a power of 2 for quick cache accesses.
    s32 items;      // The number of VALID items in the table.
    s32 resize_threshold;

    const int MIN_SIZE            = 32;
    const int LOAD_FACTOR_PERCENT = 70;

    struct Entry {
        u32         hash;
        Key_Type    key;
        Value_Type  value;
    };

    Entry *entries;

    u32  (*hash_function)(void *, s32);               // void pointer to data and length.
    bool (*comparator_function)(Key_Type, Key_Type);  // comparator function for comparing keys.
};


template <typename Key_Type>
bool default_comparator_function(Key_Type a, Key_Type b) { 
    return a == b;
}


template <typename Key_Type, typename Value_Type>
inline void table_init(Hash_Table <Key_Type, Value_Type> *table, s64 _table_size=0, bool (*given_comparator)(Key_Type, Key_Type )=NULL, u32 (*given_hash_function)(void *, s32)=NULL) {
    if (!given_hash_function) {
        table->hash_function = murmur_32;
    } else {
        table->hash_function = given_hash_function;
    }

    if (!given_comparator) {
        table->comparator_function = default_comparator_function;
    } else {
        table->comparator_function = given_comparator;
    }

    if (_table_size == 0) { _table_size = table->MIN_SIZE; }

    u32 aligned_table_size = next_power_of_two(_table_size);

    table->table_size = aligned_table_size;
    table->items      = 0;

    table->entries = (typename Hash_Table <Key_Type, Value_Type>::Entry *) calloc(table->table_size, sizeof(typename Hash_Table <Key_Type, Value_Type>::Entry));

    memset(table->entries, 0, table->table_size*sizeof(typename Hash_Table <Key_Type, Value_Type>::Entry));

    table->resize_threshold = (table->table_size * table->LOAD_FACTOR_PERCENT) / 100;
}

template <typename Key_Type, typename Value_Type>
inline void table_deinit(Hash_Table <Key_Type, Value_Type> *table) {
    free(table->entries);
}

template <typename Key_Type, typename Value_Type>
inline void table_expand(Hash_Table <Key_Type, Value_Type> *table) {
    auto *old_entries = table->entries;
    s32   old_size    = table->table_size;

    s32 new_table_size = table->table_size * 2;
    if (new_table_size < table->MIN_SIZE) {
        new_table_size = table->MIN_SIZE;
    }

    table_init(table, new_table_size);

    for (s32 i = 0; i < old_size; ++i) {
        auto *entry = &old_entries[i];
        if (entry->hash >= HASH_STATE::VALID) {
            table_add(table, entry->key, entry->value);
        }
    }

    free(old_entries);
}

template <typename Key_Type, typename Value_Type>
inline bool table_remove(Hash_Table <Key_Type, Value_Type> *table, Key_Type key) {
    u32 hash = table->hash_function((void *)&key, sizeof(key));

    if (hash < HASH_STATE::VALID) { hash += HASH_STATE::VALID; }

    u32 index = hash & (table->table_size - 1);

    while (table->entries[index].hash) {
        auto *entry = &table->entries[index];
        if (entry->hash == hash && table->comparator_function(entry->key, key)) {
            entry->hash = HASH_STATE::DELETED;
            --table->items;
            return true;
        }

        index += 1;
        if (index >= table->table_size) { index = 0; }
    }

    return false;
}

template <typename Key_Type, typename Value_Type>
inline void table_add(Hash_Table <Key_Type, Value_Type> *table, Key_Type key, Value_Type value) {
    if (table->items >= table->resize_threshold) { table_expand(table); }

    assert(table->items <= table->table_size);

    u32 hash = table->hash_function((void *)&key, sizeof(key));

    if (hash < HASH_STATE::VALID) { hash += HASH_STATE::VALID; }

    u32 index = hash & (table->table_size - 1);

    while (1) { // We should always have an empty slot.
        auto *entry = &table->entries[index];
        if (entry->hash == HASH_STATE::VACANT) {
            entry->hash  = hash;
            entry->key   = key;
            entry->value = value;
            table->items++;
            return;
        }

        index += 1;
        if (index >= table->table_size) { index = 0; }
    }
}

template <typename Key_Type, typename Value_Type>
inline Value_Type *table_find_pointer(Hash_Table <Key_Type, Value_Type> *table, Key_Type key) {
    if (!table->table_size) { return NULL; }

    u32 hash = table->hash_function((void *)&key, sizeof(key));

    if (hash <= HASH_STATE::VALID) { hash += HASH_STATE::VALID; }

    u32 index = hash & (table->table_size - 1);

    while (table->entries[index].hash) {
        auto *entry = &table->entries[index];
        if (entry->hash == hash && table->comparator_function(entry->key, key)) {
            return &entry->value;
        }

        index += 1;
        if (index >= table->table_size) { index = 0; }
    }

    return NULL;
}

template <typename Key_Type, typename Value_Type>
inline bool table_find(Hash_Table <Key_Type, Value_Type> *table, Key_Type key) {
    Value_Type *value = table_find_pointer(table, key);
    if (!value) { return false; }
    return true;
}

template <typename Key_Type, typename Value_Type>
inline void table_set(Hash_Table <Key_Type, Value_Type> *table, Key_Type key, Value_Type new_value) {
    Value_Type *old_value = table_find_pointer(table, key);
    if (old_value) {  // If there exists an old value just point the old value to the new value.
        *old_value = new_value;
    } else {
        table_add(table, key, new_value);
    }
}
