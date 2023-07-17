#pragma once 

#include "Array.h"
#include "Types.h"

template <typename T>
struct Queue { 
    // The container for the queue.
    Array <T> data;
    // You get this for free from Array but we can just track it here.
    u64 size;
};

template <typename T>
inline void queue_init(Queue<T> *queue) { 
    array_init(&queue->data);
    queue->size = 0;
}

template <typename T>
inline void queue_deinit(Queue<T> *queue) { 
    array_deinit(&queue->data);
    queue->size = 0;
}

template <typename T>
inline void queue_push(Queue<T> *queue, T value) { 
    array_push(&queue->data, value);
    ++queue->size;
}

template <typename T> 
inline T &queue_peek_front(Queue<T> *queue) { 
    return array_peek_front(&queue->data);
}

template <typename T> 
inline T &queue_peek_back(Queue<T> *queue) { 
    return array_peek_back(&queue->data);
}

template <typename T>
inline bool queue_empty(Queue<T> *queue) { 
    bool empty = (queue->size == 0);
    return empty;
}

// You can acceess this on the struct however this is here for completeness.
template <typename T>
inline u64 queue_size(Queue<T> *queue) { 
    return queue->size;
}

template <typename T>
T &queue_pop(Queue<T> *queue) { 
    if (queue->size) { 
        T &ret = array_pop_front(&queue->data);
        --queue->size;
        return ret;
    }
    
}

