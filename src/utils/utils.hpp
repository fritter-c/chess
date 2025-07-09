#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "vector.hpp"

namespace utils {
template <typename T> constexpr T abs(T value) { return (value < 0) ? -value : value; }
template <typename T> constexpr T min(T a, T b) { return (a < b) ? a : b; }
template <typename T> constexpr T max(T a, T b) { return (a > b) ? a : b; }
template <typename T> constexpr T clamp(T value, T a, T b) { return min(max(value, a), b); }
} // namespace utils
