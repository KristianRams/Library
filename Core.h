#include "Types.h"
#include <intrin.h>

void *memset(void *dest, s32 data, size_t n) {
    __stosb((u8 *)dest, (unsigned char)data, n);
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    __movsb((u8 *)dest, (u8 *)src, n);
    return dest;
}

s32 memcmp(const void *s1, const void *s2, size_t n) { 
    s32 offset = 0;
    while (offset < n) { 
        u8 *a = (u8 *)s1+offset;
        u8 *b = (u8 *)s2+offset;
        if (*a != *b) { return a - b; }
        ++offset;
    }
    return 0;
}
