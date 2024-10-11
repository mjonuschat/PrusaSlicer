#pragma once

#include <functional>

#include <libslic3r/Point.hpp>

namespace Slic3r::App::Scene {

class IImguiRenderNodeComponent
{
public:
    using Aabb2f = Eigen::AlignedBox<float, 2>;

    virtual ~IImguiRenderNodeComponent() = default;

    virtual void render_imgui(const Node& node, const Aabb2f& screen_bounding_box) const = 0;
};

class FuncImguiRenderNodeComponent : public IImguiRenderNodeComponent
{
public:
    using RenderFunc = std::function<void(const Node&, const Aabb2f&)>;

    explicit FuncImguiRenderNodeComponent(const RenderFunc render_func) : m_render_func(render_func)
    {}

    void render_imgui(const Node& node, const Aabb2f& screen_bounding_box) const override
    {
        m_render_func(node, screen_bounding_box);
    }

private:
    RenderFunc m_render_func;
};

}
