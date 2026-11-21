// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "ir/shared_ptr.h"

#include <mutex>

namespace P4::IR {

namespace {
struct Allocation {
    void *address;
    size_t size;
    Allocation *next;
};

// Keep pending allocations visible to BDWGC, without registering thread-local
// destructors whose bookkeeping can be collected before process shutdown.
std::mutex allocationMutex;
Allocation *pendingAllocations = nullptr;
}  // namespace

void shared_ptr_base::recordAllocation(void *address, size_t size) {
    std::lock_guard lock(allocationMutex);
    pendingAllocations = new Allocation{address, size, pendingAllocations};
}

void shared_ptr_base::forgetAllocation(void *address) noexcept {
    std::lock_guard lock(allocationMutex);
    for (auto **entry = &pendingAllocations; *entry; entry = &(*entry)->next) {
        if ((*entry)->address == address) {
            auto *allocation = *entry;
            *entry = allocation->next;
            delete allocation;
            return;
        }
    }
}

bool shared_ptr_base::consumeAllocation(const void *base) {
    const auto address = reinterpret_cast<uintptr_t>(base);
    std::lock_guard lock(allocationMutex);
    // Constructor arguments can allocate other nodes before the base constructor
    // runs, and virtual inheritance can place the base inside the allocation.
    for (auto **entry = &pendingAllocations; *entry; entry = &(*entry)->next) {
        const auto start = reinterpret_cast<uintptr_t>((*entry)->address);
        if (address >= start && address - start < (*entry)->size) {
            auto *allocation = *entry;
            *entry = allocation->next;
            delete allocation;
            return true;
        }
    }
    return false;
}

void *shared_ptr_base::operator new(size_t size) {
    void *address = ::operator new(size);
    try {
        recordAllocation(address, size);
    } catch (...) {
        ::operator delete(address);
        throw;
    }
    return address;
}
void *shared_ptr_base::operator new(size_t size, std::align_val_t alignment) {
    void *address = ::operator new(size, alignment);
    try {
        recordAllocation(address, size);
    } catch (...) {
        ::operator delete(address, alignment);
        throw;
    }
    return address;
}
void shared_ptr_base::operator delete(void *ptr) noexcept {
    forgetAllocation(ptr);
    ::operator delete(ptr);
}
void shared_ptr_base::operator delete(void *ptr, std::align_val_t alignment) noexcept {
    forgetAllocation(ptr);
    ::operator delete(ptr, alignment);
}

}  // namespace P4::IR
