#pragma once

// Murmur32 hash implementation

const u32 SEED = 0x58bc4716;

inline
u32 murmur_32(void *data, s32 len) { 
    const u32 c1 = 0xcc9e2d51;
    const u32 c2 = 0x1b873593;
    const u32 r1 = 15;
    const u32 r2 = 13;
    const u32 m  = 5;
    const u32 n  = 0xe6546b64;

    const u32 mix_a = 0x85ebca6b;
    const u32 mix_b = 0xc2b2ae35;

    u32 hash = SEED;
    u32 k = 0;

    u32 *d = (u32 *)data;
    s32 nblocks = len / 4;
    
    u8 *tail = (u8 *)d+nblocks*4;
    
    // Groups of 4 bytes.
    for (int i = 0; i < nblocks; ++i) {
        k = d[i];
        k *= c1;
        k = (k << r1)  | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2)))*m+n;  
    }
    
    // Read the rest 
    k = 0;
    switch (len & 3) {
    case 3:  // Fall through
        k ^= tail[2] << 16;
    case 2:  // Fall through
        k ^= tail[1] << 8;
    case 1: 
        k ^= tail[0];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;
        hash ^= k;
    }

    // finalize
    hash ^= len;
    hash ^= (hash >> 16);
    hash *= mix_a;
    hash ^= (hash >> r2);
    hash *= mix_b;
    hash ^= (hash >> 16);

    return hash;
}


