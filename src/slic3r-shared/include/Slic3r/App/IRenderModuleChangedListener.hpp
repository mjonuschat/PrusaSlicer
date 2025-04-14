#pragma once

namespace Slic3r::App {

namespace Render {
enum class ModuleType
{
    Undef,
    Plater,
    Preview,
};
}

class IRenderModuleChangedListener
{
public:
    virtual ~IRenderModuleChangedListener() = default;
    virtual void on_render_module_changed(Render::ModuleType type) = 0;
};

} // Slic3r::App

