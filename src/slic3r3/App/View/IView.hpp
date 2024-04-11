#pragma once

namespace Slic3r::App::View {
    struct IView {
        virtual ~IView() = default;

        virtual void render_imgui() = 0;
    };
}