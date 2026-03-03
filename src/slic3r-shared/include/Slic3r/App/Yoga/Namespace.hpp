#pragma once

#include "Slic3r/Domain/Types.hpp"
#include "yoga/Yoga.h"

#include <functional>

namespace Slic3r::App::Yoga {
using Vec2f = Slic3r::Domain::Vec2f;
using Vec4f = Slic3r::Domain::Vec4f;

// Used for rendering and sizing
using RenderPosFn = std::function<void(Vec2f, Vec2f)>;
using RenderFn = std::function<void(Vec2f)>;

enum class Orientation
{
    Horizontal,
    Vertical,
    VerticalReverse,
};

enum class Direction
{
    LeftToRight,
    RightToLeft
};

enum class Position
{
    Top,
    Bottom,
    Left,
    Right
};

struct Sides
{
    Sides(float size = 0.f) { left = right = top = bottom = size; }

    Sides(float horizontal, float vertical)
    {
        left = right = horizontal;
        top = bottom = vertical;
    }

    Sides(float left, float top, float right, float bottom)
        : left(left), right(right), top(top), bottom(bottom)
    {}

    Sides(Vec2f margins)
    {
        left = right = margins.x();
        top = bottom = margins.y();
    }

    inline float horizontal() const { return left + right; }

    inline float vertical() const { return top + bottom; }

    inline bool operator==(const Sides& rhs) const
    {
        return left == rhs.left && right == rhs.right && top == rhs.top && bottom == rhs.bottom;
    }
    inline bool operator!=(const Sides& rhs) const { return !(*this == rhs); }

    float left{0.f};
    float right{0.f};
    float top{0.f};
    float bottom{0.f};
};

using Margins = Sides;
using Paddings = Sides;

enum class AlignH : int
{
    Left,
    Center,
    Right
};

enum class AlignV
{
    Top,
    Center,
    Bottom
};

struct Align
{
    AlignH horizontal{AlignH::Left};
    AlignV vertical{AlignV::Top};

    bool operator==(const Align& rhs) const
    {
        return horizontal == rhs.horizontal && vertical == rhs.vertical;
    }
    bool operator!=(const Align& rhs) const { return !(*this == rhs); }

    YGAlign get_yoga_v_align() const
    {
        switch (vertical) {
        case AlignV::Top:
            return YGAlignFlexStart;
        case AlignV::Center:
            return YGAlignCenter;
        case AlignV::Bottom:
            return YGAlignFlexEnd;
        }
        return {};
    }

    YGAlign get_yoga_h_align() const
    {
        switch (horizontal) {
        case AlignH::Left:
            return YGAlignFlexStart;
        case AlignH::Center:
            return YGAlignCenter;
        case AlignH::Right:
            return YGAlignFlexEnd;
        }
        return {};
    }
};
} // namespace Slic3r::App::Yoga
