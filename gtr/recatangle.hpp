#pragma once
#ifndef RECTANGLE_HPP
#define RECTANGLE_HPP
#include "vec2.hpp"

namespace gtr {
struct rectangle {
    vec2 top_left{0.0f, 0.0f};
    vec2 bottom_right{0.0f, 0.0f};

    rectangle() = default;
    rectangle(const vec2 &top_left, const vec2 &bottom_right) : top_left(top_left), bottom_right(bottom_right) {}
    rectangle(const float x, const float y, const float width, const float height) : top_left(x, y), bottom_right(x + width, y + height) {}
    rectangle(const vec2 &top_left, const float width, const float height) : top_left(top_left), bottom_right(top_left.x + width, top_left.y + height) {}

    vec2 size() const { return bottom_right - top_left; }
    vec2 center() const { return vec2(top_left.x + size().x / 2.0f, top_left.y + size().y / 2.0f); }

    void size(const vec2 &size) {
        bottom_right.x = top_left.x + size.x;
        bottom_right.y = top_left.y + size.y;
    }
    bool contains(const vec2 &point) const { return point.x >= top_left.x && point.x <= bottom_right.x && point.y >= top_left.y && point.y <= bottom_right.y; }
};

} // namespace gtr
#endif