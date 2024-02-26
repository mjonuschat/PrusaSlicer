#pragma once

#include <memory>
#include <vector>

#include "IPanel.hpp"

namespace Slic3r::App::View {

class Frame {
protected:
    Frame() = default;
public:
    virtual ~Frame() = default;
    virtual void render_imgui();
    virtual void render_background();
protected:
    using ViewList = std::vector<std::unique_ptr<IPanel>>;

    ViewList m_views;
};

}