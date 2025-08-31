#pragma once
#ifndef VEC2_HPP
#define VEC2_HPP
#include <cmath>
namespace gtr {
struct vec2 {
    float x{0.0f};
    float y{0.0f};

    vec2() = default;
    constexpr vec2(const float x, const float y) : x(x), y(y) {}

    vec2 operator+=(const vec2 &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2 operator-=(const vec2 &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vec2 operator+(const vec2 &other) const { return {x + other.x, y + other.y}; }

    vec2 operator-(const vec2 &other) const { return {x - other.x, y - other.y}; }

    float distance(const vec2 &other) const {
        const float dx = x - other.x;
        const float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};
} // namespace gtr
#endif