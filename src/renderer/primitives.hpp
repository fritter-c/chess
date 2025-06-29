#pragma once
namespace renderer {
    struct Vec2 {
        float x, y;
    };

    struct Rectangle {
        Vec2 topLeft;
        Vec2 bottomRight;

        void Size(const Vec2 size) {
            bottomRight.x = topLeft.x + size.x ;
            bottomRight.y = topLeft.y + size.y ;
        }

        Vec2 Size() const {
            return Vec2{
                bottomRight.x - topLeft.x,
                bottomRight.y - topLeft.y
            };
        }
    };
}
