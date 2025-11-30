#pragma once
#include <cstdlib>
#include <memory>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <new>
#include <algorithm>

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

    PoolAllocator(const PoolAllocator& other) noexcept {
    }

    template <class U>
    PoolAllocator(const PoolAllocator<U>&) noexcept {
    }

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
            if (free_list) {
                Slot* s = free_list;
                free_list = s->next;
                ++used_slots;
                return reinterpret_cast<pointer>(s);
            }
            if (!ensure_free_slot()) {
                expand(std::max<size_type>(1, default_block_size));
            }
            Slot* s = pop_from_block();
            ++used_slots;
            return reinterpret_cast<pointer>(s);
        } else {
            size_type bytes = n * sizeof(T);
            void* p = std::malloc(bytes);
            if (!p) throw std::bad_alloc();
            large_allocs.push_back(p);
            return reinterpret_cast<pointer>(p);
        }
    }

    void deallocate(pointer p, size_type n) noexcept {
        if (!p) return;
        if (n == 1) {
            Slot* s = reinterpret_cast<Slot*>(p);
            s->next = free_list;
            free_list = s;
            --used_slots;
        } else {
            auto it = std::find(large_allocs.begin(), large_allocs.end(), reinterpret_cast<void*>(p));
            if (it != large_allocs.end()) {
                std::free(*it);
                large_allocs.erase(it);
            }
        }
    }

    void reserve(size_type new_cap) {
        if (new_cap <= total_slots) return;
        expand(new_cap - total_slots);
    }

    void release_all() noexcept {
        for (void* b : blocks) std::free(b);
        blocks.clear();
        for (void* p : large_allocs) std::free(p);
        large_allocs.clear();
        free_list = nullptr;
        total_slots = used_slots = 0;
    }

    bool operator==(const PoolAllocator& other) const noexcept { return this == &other; }
    bool operator!=(const PoolAllocator& other) const noexcept { return !(*this == other); }

private:
    struct Slot { Slot* next; };
    Slot* free_list = nullptr;

    std::vector<void*> blocks;
    std::vector<void*> large_allocs;
    size_type total_slots = 0;
    size_type used_slots = 0;

    static constexpr size_type default_block_size = 16;

    void expand(size_type count) {
        size_type block_slots = std::max(count, default_block_size);
        size_type bytes = block_slots * sizeof(T);
        void* raw = std::malloc(bytes);
        if (!raw) throw std::bad_alloc();
        blocks.push_back(raw);
        char* base = reinterpret_cast<char*>(raw);
        for (size_type i = 0; i < block_slots; ++i) {
            Slot* s = reinterpret_cast<Slot*>(base + i * sizeof(T));
            s->next = free_list;
            free_list = s;
        }
        total_slots += block_slots;
    }

    bool ensure_free_slot() {
        if (free_list) return true;
        expand(default_block_size);
        return free_list != nullptr;
    }

    Slot* pop_from_block() {
        Slot* s = free_list;
        free_list = s->next;
        return s;
    }
};
