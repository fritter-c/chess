#pragma once
#include <cstdint>
#include <stacktrace>
#include <unordered_map>
#include <mutex>
#include "assert.hpp"
namespace gtr {
template <typename T> struct tracking_allocator {
    using value_type = T;
    using pointer_type = T *;
    using size_type = uint64_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment  = std::true_type;
    using propagate_on_container_copy_assignment  = std::true_type;
    using propagate_on_container_swap             = std::true_type;
    using is_always_equal                         = std::true_type;

    static std::unordered_map<void*, size_type> allocations;
    static std::mutex                           alloc_mutex;

    static T *allocate(uint64_t n) { return malloc(n); }
    static T* reallocate(T *ptr, uint64_t size, uint64_t old_size) { return realloc(ptr, size, old_size); }
    static void deallocate(T *p, uint64_t n) { return free(p, n); }

    static T *malloc(const size_type size) {
        std::lock_guard lock(alloc_mutex);
        std::puts("TRACKING MALLOC");
        std::puts(to_string(std::stacktrace::current()).c_str());
        auto new_ptr =  static_cast<T *>(std::malloc(size * sizeof(T)));
        allocations.insert({new_ptr,size});
        return new_ptr;
    }

    static T *realloc(T *ptr, const size_type size, const size_type old_size) {
        (void)old_size;
        std::lock_guard lock(alloc_mutex);
        if (!ptr) return malloc(size);
        std::puts("TRACKING REALLOC");
        std::puts(to_string(std::stacktrace::current()).c_str());
        auto alloc = allocations.find(ptr);
        Assert(alloc != nodes.end(), "Allocation not found");
        Assert(alloc->size == old_size, "Allocation size mismatch");
        allocations.erase(alloc);
        auto new_ptr = static_cast<T *>(std::realloc(ptr, size * sizeof(T)));
        allocations.insert({new_ptr,size});
        return new_ptr;
    }

    static void free(T *ptr, const size_type size) {
        (void)size;
        std::lock_guard lock(alloc_mutex);
        if (!ptr) return;
        std::puts("TRACKING FREE");
        std::puts(to_string(std::stacktrace::current()).c_str());
        auto alloc = allocations.find(ptr);
        Assert(alloc != nodes.end(), "Allocation not found");
        Assert(alloc->size == size, "Allocation size mismatch");
        allocations.erase(alloc);
        std::free(ptr);
    }

    constexpr tracking_allocator() noexcept = default;
    template<typename U> constexpr tracking_allocator(tracking_allocator<U> const&) noexcept {}
    template<typename U> struct rebind { using other = tracking_allocator<U>; };
    bool operator==(tracking_allocator const&) const noexcept { return true; }
    bool operator!=(tracking_allocator const&) const noexcept { return false; }
};
template<typename T>
std::unordered_map<void*, typename tracking_allocator<T>::size_type>
  tracking_allocator<T>::allocations;

template<typename T>
std::mutex tracking_allocator<T>::alloc_mutex;
} // namespace gtr