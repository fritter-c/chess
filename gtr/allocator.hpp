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
    using propagate_on_container_move_assignment = std::false_type; // Does not manage state, so no need to propagate
    using propagate_on_container_copy_assignment = std::false_type; // Does not manage state, so no need to propagate
    using propagate_on_container_swap = std::false_type;            // Does not manage state, so no need to propagate
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type; // Stateless allocator, always equal
    constexpr c_allocator select_on_container_copy_construction() const noexcept { return *this; }

    T *allocate(size_type n) { return malloc(n); }
    T *reallocate(T *ptr, const size_type size, const size_type old_size) { return realloc(ptr, size, old_size); }
    void deallocate(T *p, const size_type n) { return free(p, n); }

    T *malloc(const size_type size) { return static_cast<T *>(std::malloc(size * sizeof(T))); }
    T *realloc(T *ptr, const size_type size, const size_type old_size) {(void)old_size;return static_cast<T *>(std::realloc(ptr, size * sizeof(T)));}
    void free(T *ptr, const size_type size) { (void)size, std::free(ptr); }

    constexpr c_allocator() noexcept = default;
    constexpr ~c_allocator() = default;
    template <typename U> explicit constexpr c_allocator(const c_allocator<U> &) noexcept {}; // Required to be used as a CPP allocator
    template <typename U> explicit constexpr c_allocator(c_allocator<U> &&) noexcept {}; // Required to be used as a CPP allocator
    template <class U> bool operator==(const c_allocator<U> &) const noexcept { return true; }
    template <class U> bool operator!=(const c_allocator<U> &) const noexcept { return false; }
    template <class U>  struct rebind { using other = c_allocator<U>; };
};

#if defined(_MSC_VER) || defined(_WIN64) || defined(_WIN32)
#define ALIGNED_ALLOC(size, alignment) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
#define ALIGNED_ALLOC(size, alignment) aligned_alloc(alignment, size)
#define ALIGNED_FREE(ptr) std::free(ptr)
#endif

template <typename T, uint64_t Alignment = alignof(max_align_t) > struct aligned_allocator {
    using value_type = T;
    using pointer_type = T *;
    using size_type = uint64_t;
    using propagate_on_container_move_assignment = std::false_type; // Does not manage state, so no need to propagate
    using propagate_on_container_copy_assignment = std::false_type; // Does not manage state, so no need to propagate
    using propagate_on_container_swap = std::false_type;            // Does not manage state, so no need to propagate
    using difference_type = std::ptrdiff_t;
    using is_always_equal = std::true_type; // Stateless allocator, always equal
    constexpr aligned_allocator select_on_container_copy_construction() const noexcept { return *this; }

    T *allocate(size_type n) { return malloc(n); }
    T *reallocate(T *ptr, const size_type size, const size_type old_size) { return realloc(ptr, size, old_size); }
    void deallocate(T *p, const size_type n) { return free(p, n); }

    T *malloc(uint64_t size) { return static_cast<T *>(ALIGNED_ALLOC(size * sizeof(T), Alignment)); }
    T *realloc(T *ptr, uint64_t size, uint64_t old_size) {auto new_ptr = ALIGNED_ALLOC(size, Alignment); memcpy(new_ptr, ptr, old_size); ALIGNED_FREE(ptr); return new_ptr; }
    void free(void *ptr, uint64_t size) { (void)size, ALIGNED_FREE(ptr); }

    constexpr aligned_allocator() noexcept = default;
    ~aligned_allocator() = default;
    template <class U>explicit constexpr aligned_allocator(aligned_allocator<U,Alignment> &&) noexcept {}
    template <class U> explicit constexpr aligned_allocator(const aligned_allocator<U,Alignment> &) noexcept {}
    template <class U> bool operator==(const aligned_allocator<U,Alignment> &) const noexcept { return true; }
    template <class U> bool operator!=(const aligned_allocator<U,Alignment> &) const noexcept { return false; }
    template <typename U> struct rebind { using other = aligned_allocator<U, Alignment>;};
};
} // namespace gtr
#endif