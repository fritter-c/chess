#pragma once
#ifndef CONTAINER_BASE_HPP
#define CONTAINER_BASE_HPP
#include <utility>
namespace gtr {
template <class Allocator> struct container_base : Allocator {
    explicit container_base(const Allocator &alloc = Allocator()) : Allocator(alloc) {};
    explicit container_base(Allocator &&alloc) noexcept : Allocator(std::move(alloc)) {};
    const Allocator &allocator() const noexcept { return *this; }
    Allocator &allocator() noexcept { return *this; }
};
} // namespace gtr
#endif // !CONTAINER_BASE_H
