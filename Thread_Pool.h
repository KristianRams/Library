#pragma once

#include "Types.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector> 
#include <atomic>
#include <queue>
#include <functional>

struct Thread_Pool { 
    std::condition_variable condition; 
    std::atomic<bool> thread_pool_active;
    std::queue<std::function<void()>> function_queue;
    std::mutex mutex;

    std::vector<std::thread> threads;
    s64 number_of_threads;
};

void thread_function(Thread_Pool *thread_pool) { 
    std::function<void()> function;

    while (true) {
        std::unique_lock<std::mutex> lock(thread_pool->mutex);
        thread_pool->condition.wait(lock, [thread_pool]() {
                return !thread_pool->function_queue.empty() || !thread_pool->thread_pool_active.load(std::memory_order_seq_cst);
            });

        // If the thread pool is marked !active or inactive and the function queue is empty then we're done.
        if (!thread_pool->thread_pool_active.load(std::memory_order_seq_cst) && thread_pool->function_queue.empty()) { return; }

        // Pop the function off the queue and execute it.
        function = thread_pool->function_queue.front(); 
        thread_pool->function_queue.pop();
        function();
    }
}

void init(Thread_Pool *thread_pool, u64 number_of_threads) {
    const s64 max_threads = std::thread::hardware_concurrency();
    // Cap the number of threads in the pool to the max reported by 
    if (number_of_threads > max_threads) { number_of_threads = max_threads; }

    // Protected against 0 threads
    number_of_threads = (!number_of_threads) ? 1 : number_of_threads;

    thread_pool->number_of_threads = number_of_threads;
    thread_pool->threads.reserve(thread_pool->number_of_threads);

    for (s64 i = 0; i < thread_pool->number_of_threads; ++i) { 
        thread_pool->threads.push_back(std::thread(&thread_function, thread_pool)); 
    }

    thread_pool->thread_pool_active.store(true, std::memory_order_seq_cst);
}


void process(Thread_Pool *thread_pool, std::function<void()> function) { 
    std::unique_lock<std::mutex> lock(thread_pool->mutex); 
    thread_pool->function_queue.push(function);
    thread_pool->condition.notify_one();
}

void deinit(Thread_Pool *thread_pool) { 
    thread_pool->thread_pool_active.store(false, std::memory_order_seq_cst);
    thread_pool->condition.notify_all();
    for (s64 i = 0; i < thread_pool->number_of_threads; ++i) { 
        if (thread_pool->threads[i].joinable()) { 
            thread_pool->threads[i].join();
        }
    }
}
