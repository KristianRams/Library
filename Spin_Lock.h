#pragma once

#include "Types.h"
#include <atomic>
#include <thread>
#include <vector>
#include <stdarg.h>
#include <assert.h>
#include <iostream> 

#if 1
// Another way of getting rdtsc from 
// https://stackoverflow.com/questions/13772567/how-to-get-the-cpu-cycle-count-in-x86-64-from-c

#include <stdint.h>

//  Windows
#ifdef _WIN32

#include <intrin.h>
uint64_t rdtsc(){
    return __rdtsc();
}

//  Linux/GCC
#else

uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

#endif // Windows

#endif 


#ifdef linux
#include <sched.h>

extern inline unsigned long long
__attribute__((always_inline))
__rdtsc () { return __builtin_ia32_rdtsc (); }

#endif

#ifdef _WIN32
#include <intrin.h>
#include "Windows.h"
#pragma intrinsic(__rdtsc)
#endif 



struct Spin_Lock { 
    std::atomic<bool> locked    = {false};
    const char       *lock_name = nullptr;
    std::atomic<s64>  core_id   = {-1}; // 0 is valid.
};

s64 get_core_id() { 
#ifdef linux
    return sched_getcpu();
#endif 

#ifdef _WIN32
    return GetCurrentProcessorNumber();
#endif 

    return -1;
}

bool holding(Spin_Lock *spin_lock) { 
    if (spin_lock->locked.load(std::memory_order_seq_cst) && 
        spin_lock->core_id.load(std::memory_order_seq_cst) == get_core_id()) {
        return true;
    }
    return false;
}

void lock(Spin_Lock *spin_lock) { 
    u64 timeout  = 0;
    u64 threshold = 2 << 10;

    while (1) {
        
        if (!spin_lock->locked.exchange(true, std::memory_order_seq_cst)) { break; }

        while (spin_lock->locked.load(std::memory_order_seq_cst)) {  

            if (threshold > 0) {
                
                for (int i = 0; i < threshold; ++i) { 
                    // Spin lock hint to compiler.
#ifdef linux
                    __builtin_ia32_pause();
#endif

#ifdef _WIN32
                    _mm_pause();
#endif

                    //if (spin_lock->locked.load(std::memory_order_seq_cst)) { goto critial_section; }
                    threshold--;
                }
                
            } else { // fall back to TSC 
                if (timeout == 0) {
                    // Configurable timeout (1 second on my 2.6Ghz processor).
                    timeout = rdtsc() + 2600000000;
                } else { 
                    if (rdtsc() >= timeout) { 
                        assert(false && "Lock timeout error");
                    }
                }
            }

            // Schedule another thread to run.
            // スリープループによる消費電力とパフォーマンスの改善
            std::this_thread::yield();
        }
    }

 critial_section:
    // Mutual exclusion has been reached.
    spin_lock->core_id.store(get_core_id(), std::memory_order_seq_cst);
}

void unlock(Spin_Lock *spin_lock) { 
    spin_lock->locked.store(false, std::memory_order_release);
}

void init(Spin_Lock *spin_lock, const char *name="Spin_Lock") { 
    spin_lock->lock_name = name;
}

// Nop here for completion.
void deinit(Spin_Lock *spin_lock) {  return; }
