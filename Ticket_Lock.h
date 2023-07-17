#pragma once

#include "Types.h"

#include <atomic>
#include <thread>
#include <vector>

// Super naive and most common way of implementing ticket locks
// without any optmizations.
// A `fair` spin lock based on tickets.
struct Ticket_Lock {
    std::atomic<int> ticket {0}; // Next ticket to be assigned.
    // Currently serving.
    // @Note: Modifying grant does not need to be atomic. Given the fact
    // that Ticket_Release is called after Ticket_Acquire, and another thread can only make progress
    // once the first thread increments grant. (We get thread safety on grant by definition).
    int grant = 0;
};

void Ticket_Acquire(Ticket_Lock *L) {
    auto tx = L->ticket.fetch_add(1, std::memory_order_seq_cst);

    // If the ticket equals grant then we know it is this threads time to be served.
    while (tx != L->grant) {} //L->grant.load(std::memory_order_seq_cst)) { }

    // We have reached mutual exclusion at this point.
}

void Ticket_Release(Ticket_Lock *L) {
    ++L->grant;
    //grant.fetch_add(1, std::memory_order_seq_cst);
}

