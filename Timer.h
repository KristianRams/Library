#include "Types.h"

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <stdio.h>

#define MAX_TIMING_COUNT 100
u64 timings[MAX_TIMING_COUNT] = {};

#ifdef _WIN32 // Windows
u64 rdtsc() { return __rdtsc(); }
#else // Linux/GCC
u64 rdtsc() { 
    unsigned int low,high;
    __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high)); 
    return ((u64)high << 32) | low;
}
#endif
