#pragma once
#include <cstdlib>
#include <memory>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <new>

// A small pool allocator that supports:
// - constructor(initial_capacity) to reserve elements
// - allocate(n) / deallocate(ptr,n) compatible with allocator_traits
// - automatic expansion: allocate will create a new block if needed
// - per-element deallocation: for n==1 we push the slot to free-list
// - full release in destructor
//
// Note: This allocator is stateful (holds blocks and free list).
// It is kept simple for educational/demo usage.

template <typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <class U> struct rebind { using other = PoolAllocator<U>; };

    PoolAllocator(size_type initial_capacity = 0) noexcept {
        if (initial_capacity > 0) reserve(initial_capacity);
    }

    // copy ctor - stateful: create empty allocator except when copying manually
    PoolAllocator(const PoolAllocator& other) noexcept {
        // intentionally leave empty: copying allocator does NOT duplicate blocks
        // This is a design choice: container copy semantics control propagation.
    }

    // move ctor
    PoolAllocator(PoolAllocator&& other) noexcept {
        blocks = std::move(other.blocks);
        free_list = other.free_list;
        total_slots = other.total_slots;
        used_slots = other.used_slots;
        other.free_list = nullptr;
        other.total_slots = other.used_slots = 0;
    }

    ~PoolAllocator() noexcept {
        release_all();
    }

    pointer allocate(size_type n) {
        if (n == 0) return nullptr;
        if (n == 1) {
            // try free list
            if (free_list) {
                Slot* s = free_list;
                free_list = s->next;
                ++used_slots;
                return reinterpret_cast<pointer>(s);
            }
            // else try to take from current block if space
            if (!ensure_free_slot()) {
                // allocate a new block
                expand(std::max<size_type>(1, default_block_size));
            }
            Slot* s = pop_from_block();
            ++used_slots;
            return reinterpret_cast<pointer>(s);
        } else {
            // For simplicity, for n>1 allocate raw memory with malloc/new
            // but we will try to allocate a contiguous memory block from a new block
            // We allocate a dedicated block large enough for n elements
            size_type bytes = n * sizeof(T);
            void* p = std::malloc(bytes);
            if (!p) throw std::bad_alloc();
            // store pointer in allocated_chunks to free later
            large_allocs.push_back(p);
            // Note: we don't track per-element free in this path
            return reinterpret_cast<pointer>(p);
        }
    }

    void deallocate(pointer p, size_type n) noexcept {
        if (!p) return;
        if (n == 1) {
            // push back to free list
            Slot* s = reinterpret_cast<Slot*>(p);
            s->next = free_list;
            free_list = s;
            --used_slots;
        } else {
            // large block allocated via malloc
            auto it = std::find(large_allocs.begin(), large_allocs.end(), reinterpret_cast<void*>(p));
            if (it != large_allocs.end()) {
                std::free(*it);
                large_allocs.erase(it);
            } else {
                // unknown pointer - ignore or crash
            }
        }
    }

    // reserve capacity for at least new_cap elements
    void reserve(size_type new_cap) {
        if (new_cap <= total_slots) return;
        expand(new_cap - total_slots);
    }

    // free all memory blocks
    void release_all() noexcept {
        for (void* b : blocks) std::free(b);
        blocks.clear();
        for (void* p : large_allocs) std::free(p);
        large_allocs.clear();
        free_list = nullptr;
        total_slots = used_slots = 0;
    }

    // comparison operators required by some containers (stateful)
    bool operator==(const PoolAllocator& other) const noexcept { return this == &other; }
    bool operator!=(const PoolAllocator& other) const noexcept { return !(*this == other); }

private:
    // internal slot for free list (reuse the memory of T)
    struct Slot { Slot* next; };
    Slot* free_list = nullptr;

    std::vector<void*> blocks;         // allocated blocks
    std::vector<void*> large_allocs;   // dedicated allocations for >1 requests
    size_type total_slots = 0;         // total slots available across blocks
    size_type used_slots = 0;

    static constexpr size_type default_block_size = 16;

    // allocate a new block with at least 'count' slots
    void expand(size_type count) {
        size_type block_slots = std::max(count, default_block_size);
        size_type bytes = block_slots * sizeof(T);
        void* raw = std::malloc(bytes);
        if (!raw) throw std::bad_alloc();
        blocks.push_back(raw);
        // push all slots into free_list
        char* base = reinterpret_cast<char*>(raw);
        for (size_type i = 0; i < block_slots; ++i) {
            Slot* s = reinterpret_cast<Slot*>(base + i * sizeof(T));
            s->next = free_list;
            free_list = s;
        }
        total_slots += block_slots;
    }

    // ensure at least one free slot exists; if not, expand
    bool ensure_free_slot() {
        if (free_list) return true;
        expand(default_block_size);
        return free_list != nullptr;
    }

    // pop one slot from free_list (assumes exists)
    Slot* pop_from_block() {
        Slot* s = free_list;
        free_list = s->next;
        return s;
    }
};
