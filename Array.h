#pragma once

#include "Types.h"
#include <assert.h>
#include <string.h> 

#define MAX_ARRAY_INDEX 0x7fffffff

#define ARRAY_GROWTH_FORMULA(x) (2*(x) + 8)

template <typename T>
struct Array {
    T *data      = NULL;
    s32 size     = 0;  // The index is always size-1 when size isn't 0.
    s32 capacity = 0;
    
    // Queue tracking
    s32 front = 0;

    T &operator [](const s32 index) {
        assert(index >= 0); 
        assert(index < MAX_ARRAY_INDEX);
        assert((s32)index < size); 
        return data[index]; 
    }

    const T &operator [](const s32 index) const { 
        assert(index >= 0); 
        assert (index < MAX_ARRAY_INDEX);
        assert((s32)index < size);
        return data[index];
    }

    // Iterator on the class itself. 
    T *begin() { 
        if (!data) { return NULL; } // For symmetry with the rest of the iterator calls.
        return data;
    }
        
    T *begin() const { 
        if (!data) { return NULL; }
        return data;
    }
        
    T *end() { 
        if (!data) { return NULL; }
        return data + size;
    }
        
    T *end() const { 
        if (!data) { return NULL; }
        return data + size;
    }

};

template <typename T>
void array_init(Array <T> *array) { 
    auto default_capacity = ARRAY_GROWTH_FORMULA(0);
    array->data = new T[default_capacity]{};
    array->capacity = default_capacity;
    array->front = 0;
}

template <typename T>
void array_init(Array <T> *array, s32 size) { 
    if (size > 0) { 
        array->data     = new T[size]{};
        array->size     = size;
        array->capacity = size;
    }
}

template <typename T>
void array_deinit(Array <T> *array) { 
    if (array->data) { 
        delete[] array->data; 
        array->data = NULL;
    }
    array->size     = 0;
    array->capacity = 0;
    array->front    = 0;
}

template <typename T>
void array_reserve(Array <T> *array, s32 want_capacity) { 
    // Do nothing if the want_capacity is smaller than the current capacity
    if (array->capacity >= want_capacity) { return; }

    array_allocate_and_copy(array, want_capacity);
}

template <typename T> 
void array_add(Array <T> *array, const T &value) { 
    // If we do not have enough space in the array then we 
    // need to apply the growth formula and copy over the data.
    if (array->capacity == array->size) { 
        array_mutate(array, 0); // We don't want to expand by much.
    }
    
    array->data[array->size] = value;
    array->size++;
}

// Same as pop but doesn't decrement the size
template <typename T>
inline T &array_peek(Array <T> *array) { 
    assert(array->size > 0); 
    T &result = array->data[array->size-1];
    return result; 
}

// Same as peek but returns a pointer.
template <typename T>
inline T *array_peek_pointer(Array <T> *array) { 
    assert(array->size > 0); 
    T *result = &array->data[array->size-1]; 
    return result; 
}


// Here for completeness with array_pop();
template <typename T> 
inline void array_push(Array <T> *array, const T &value) { 
    array_add(array, value);
}

template <typename T>
inline T &array_peek_front(Array <T> *array) { 
    assert(array->size > 0); 
    T &result = array->data[array->front];
    return result;
}

template <typename T>
inline T &array_peek_back(Array <T> *array) { 
    return array_peek(array);
}

template <typename T> 
inline T &array_pop_front(Array <T> *array) { 
    assert(array->size > 0); 
    T &result = array->data[0];
    // Eeeeh
    ~array->data[0];
    array->front++;
    return result;
}

//
// Internal functions
//
template <typename T>
static void array_mutate(Array <T> *array, s32 want_capacity) {
    s32 new_capacity = ARRAY_GROWTH_FORMULA(array->capacity);
    if (new_capacity < want_capacity) {
        new_capacity = want_capacity;
    }
    array_allocate_and_copy(array, new_capacity);
}

template <typename T>
void array_resize(Array <T> *array, s32 size) { 
    array_allocate_and_copy(array, size);
    array->size = size;
}

template <typename T>
static void array_allocate_and_copy(Array <T> *array, s32 new_capacity) { 
    // If the capacities are the same then don't do anything.
    if (new_capacity == array->capacity) { return; }

    // If we want to resize the array to a smaller capacity.
    // Here capacity and size are overloaded since we are shrinking.
    if (new_capacity < array->capacity) {
        auto old_size = array->size;
        auto new_size = new_capacity;
        auto bytes_to_clear = (old_size - new_size)*sizeof(T);
        memset(array->data+new_size, 0x00, bytes_to_clear);
        // Don't update the capacity as this will invalidate 
        // possible iterators. 
        return;
    }

    if (new_capacity > array->capacity) { 
        T *new_data = new T[new_capacity];
        memcpy(new_data, array->data, array->size*sizeof(T));
        delete[] array->data;
        array->data     = new_data;
        array->capacity = new_capacity;
        return;
    }
}
