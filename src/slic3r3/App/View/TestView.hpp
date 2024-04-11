#pragma once

#include <cstddef>

#include "IView.hpp"

namespace Slic3r::App::View {
class TestView : public IView {
public:
    void render_imgui() override;

    float value {0};
    constexpr static size_t str_capacity = 64;
    char str[str_capacity] {0};
};
}