#pragma once
#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP
#include <cstdint>
#include <cstdlib>
#include <type_traits>
namespace gtr {
template <typename T> struct c_allocator {
    using value_type = T;
    using pointer_type = T *;
    using size_type = uint64_t;
    using propagate_on_container_move_assignment = std::true_type;
    using difference_type = std::ptrdiff_t;
    constexpr c_allocator select_on_container_copy_construction() const noexcept { return *this; }
    T *allocate(size_type n) { return malloc(n); }
    void deallocate(T *p, size_type n) { return free(p, n); }
    T *reallocate(T *ptr, size_type size, size_type old_size) { return realloc(ptr, size, old_size); }
    template <class U> bool operator==(const c_allocator<U> &) const noexcept { return true; }
    template <class U> bool operator!=(const c_allocator<U> &) const noexcept { return false; }
    T *malloc(size_type size) { return static_cast<T *>(std::malloc(size * sizeof(T))); }
    T *realloc(T *ptr, size_type size, size_type old_size) {
        (void)old_size;
        return static_cast<T *>(std::realloc(ptr, size * sizeof(T)));
    }
    void free(T *ptr, size_type size) { (void)size, std::free(ptr); }
    constexpr c_allocator() noexcept = default;
    constexpr ~c_allocator() = default;
};
} // namespace gtr
#endif