#pragma once

#include "Vector2.h"
#include <cmath>

namespace quadtree
{

template<typename T>
class Box
{
public:
    T left;
    T top;
    T width; // Must be positive
    T height; // Must be positive

    constexpr Box(T Left = 0, T Top = 0, T Width = 0, T Height = 0) noexcept :
        left(Left), top(Top), width(Width), height(Height)
    {

    }

    constexpr Box(const Vector2<T>& position, const Vector2<T>& size) noexcept :
        left(position.x), top(position.y), width(size.x), height(size.y)
    {

    }

    constexpr T getRight() const noexcept
    {
        return left + width;
    }

    constexpr T getBottom() const noexcept
    {
        return top + height;
    }

    constexpr Vector2<T> getTopLeft() const noexcept
    {
        return Vector2<T>(left, top);
    }

    constexpr Vector2<T> getCenter() const noexcept
    {
        return Vector2<T>(left + width / 2, top + height / 2);
    }

    constexpr Vector2<T> getSize() const noexcept
    {
        return Vector2<T>(width, height);
    }

    constexpr bool contains(const Box<T>& box) const noexcept
    {
        return left <= box.left && box.getRight() <= getRight() &&
            top <= box.top && box.getBottom() <= getBottom();
    }

    constexpr bool intersects(const Box<T>& box) const noexcept
    {
        return !(left >= box.getRight() || getRight() <= box.left ||
            top >= box.getBottom() || getBottom() <= box.top);
    }
};


template <typename Float>
inline Float distance (const Box<Float>& a, const Box<Float>& b)
{
    const Float& al = a.left; //a left
    const Float ar = a.left + a.width; //a right
    const Float& at = a.top; //a top
    const Float ab = a.top + a.height; //a bottom
    const Float& bl = b.left; //b left
    const Float br = b.left + b.width; //b right
    const Float& bt = b.top; //b top
    const Float bb = b.top + b.height; //b bottom

    if (ar < bl && ab < bt)
        return std::sqrt((bl - ar) * (bl - ar) + (bt - ab) * (bt - ab));
    else if (al > br && ab < bt)
        return std::sqrt((al - br) * (al - br) + (bt - ab) * (bt - ab));
    else if (al > br && at > bb)
        return std::sqrt((al - br) * (al - br) + (at - bb) * (at - bb));
    else if (ar < bl && at > bb)
        return std::sqrt((bl - ar) * (bl - ar) + (at - bb) * (at - bb));
    else if (ar < bl)
        return bl - ar;
    else if (ab < bt)
        return bt - ab;
    else if (al > br)
        return al - br;
    else if (at > bb)
        return at - bb;
    else
        return Float{};
}

}
