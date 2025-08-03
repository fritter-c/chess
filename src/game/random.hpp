#pragma once
#include <cstdint>
namespace game::detail {
struct RandomGenerator {
    uint64_t seed{0x123456789ABCDEF0ULL};
    RandomGenerator() = default;
    constexpr RandomGenerator(const uint64_t s) : seed(s) {}
    constexpr uint64_t next() {
        seed ^= seed << 21;
        seed ^= seed >> 35;
        seed ^= seed << 4;
        return seed;
    }
    constexpr uint64_t operator()() { return next(); }

    constexpr uint64_t sparse_rand(){
        return next() & next() & next();
    }
};
} // namespace detail