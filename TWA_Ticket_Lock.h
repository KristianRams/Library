#include "Types.h"

#include <stdio.h>
#include <assert.h>
#include <atomic>
#include <vector> 
#include <thread> 

// A modified Ticket Lock implementation based on the following paper, 
// https://arxiv.org/abs/1810.01573

// Threshold is modifiable however most optimial value found is 1. 
const u64 long_term_threshold = 1;  

// 4096 based on paper.
const u64 array_size = 2 << 11;  // @Incomplete: Change to number of CPUs on system.

// Shared amongst all threads. 
u64 wait_array[array_size] = {}; 

// Ticket Lock augmented with a waiting array. (TWA)
struct Ticket_Lock {
    std::atomic<u64> ticket {0}; // Next ticket to be assigned.
    std::atomic<u64> grant  {0}; // Currently serving. 
};

// From paper...
// "We multiply the ticket value by 127 and then EXCLUSIVE-OR
// that result with the address of the lock, and then mask with
// 4096 − 1 to form an index into the waiting array."
u64 Hash(Ticket_Lock *L, u64 tx) { 
    assert(array_size > 0);
    const u64 mask = array_size-1; // Mask is an index into the array.
    u64 step1      = tx * 127;
    u64 step2      = step1 ^ (u64)L;
    u64 step3      = step2 & mask;
    return step3;
}

void TWA_ticket_acquire(Ticket_Lock *L) { 
    auto tx = L->ticket.fetch_add(1, std::memory_order_seq_cst); // modify ticket
    auto dx = tx - L->grant.load(std::memory_order_seq_cst);     // fetch grant

    if (dx == 0) {  // Uncontended acquisition (Enter critial section) AKA Lock acquistion fast path.
        return;
    }

    // Enter long-term waiting phase.
    if (dx > long_term_threshold) {
        auto at = Hash(L, tx); // Hash ticket value and get index into waiting array.
        assert(at < array_size);
        while (1) { 
            auto u = wait_array[at]; 
            dx = tx - L->grant.load(std::memory_order_seq_cst); // recheck for data race (ticket - grant - 1 = number of waiters)
            assert(dx >= 0);
            if (dx <= long_term_threshold) { break; } // revert to short term waiting if grant is sufficiently near ticket.
            while (wait_array[at] == u) { // Long-term waiting spining on whether the value in the wait array changes. (Value inside the wait array is meaningless).
                __builtin_ia32_pause();
                
                std::this_thread::yield(); // yield time quantum to another thread.
            }
        }
    }

    // Enter short-term waiting phase.
    while (tx != L->grant.load(std::memory_order_seq_cst)) {
        __builtin_ia32_pause(); 
        
        // @Note: My own modification of the algorithm (testing shows huge performance gains by doing this. - K. Ramsamooj 7-27-22
        std::this_thread::yield(); // Be polite and yield your time quantum to another thread. 
    }
}

void TWA_ticket_release(Ticket_Lock *L) { 
    // Notify any thread that is in the short-term waiting phase.
    
    L->grant.store(L->grant.load(std::memory_order_seq_cst)+1, std::memory_order_seq_cst);
    auto k = L->grant.load(std::memory_order_seq_cst);

    // Notify any long-term waiters.
    __atomic_fetch_add(&wait_array[Hash(L, k+long_term_threshold)], 1, __ATOMIC_SEQ_CST);
}
