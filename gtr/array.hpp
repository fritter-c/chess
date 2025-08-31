#pragma once
#ifndef ARRAY_HPP
#define ARRAY_HPP
#include <iterator>
namespace gtr {
template <class T, size_t N> struct array {
    static_assert(N > 0, "Array size must be greater than 0");
    using size_type = size_t;
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;
    using iterator = T *;
    using const_iterator = const T *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    T m_data[N];

    constexpr T &operator[](size_type index) { return m_data[index]; }
    constexpr const T &operator[](size_type index) const { return m_data[index]; }
    constexpr T &at(size_type index) { return m_data[index]; }
    constexpr const T &at(size_type index) const { return m_data[index]; }
    constexpr T &front() { return m_data[0]; }
    constexpr const T &front() const { return m_data[0]; }
    constexpr T &back() { return m_data[N - 1]; }
    constexpr const T &back() const { return m_data[N - 1]; }
    constexpr T *data() { return m_data; }
    constexpr const T *data() const { return m_data; }

    constexpr size_type size() const { return N; }

    constexpr iterator begin() { return m_data; }
    constexpr const_iterator begin() const { return m_data; }
    constexpr iterator end() { return m_data + N; }
    constexpr const_iterator end() const { return m_data + N; }
    constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }
    constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    constexpr reverse_iterator rend() { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
};
// Deducers
template <class T, class... U>
requires ((std::is_same_v<std::decay_t<T>, std::decay_t<U>> && ...))
array(T, U...) -> array<std::decay_t<T>, 1 + sizeof...(U)>;
} // namespace gtr

#endif
