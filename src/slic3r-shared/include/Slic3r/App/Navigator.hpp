#pragma once
#include "Slic3r/App/IRenderModuleChangedListener.hpp"

namespace Slic3r::App {

namespace Plater {
    class PlaterRenderModule;
}

namespace Preview {
    class PreviewRenderModule;
}

namespace Platform {
    class AbstractRenderCanvas;
}

class Navigator: public IRenderModuleChangedListener
{
public:
    ~Navigator();

    void on_init(Plater::PlaterRenderModule& plater_module, 
                 Preview::PreviewRenderModule& preview_module, 
                 Platform::AbstractRenderCanvas& canvas);

    void on_render_module_changed(Render::ModuleType type) override;

private:

    Plater::PlaterRenderModule*     m_plater_module     {nullptr};
    Preview::PreviewRenderModule*   m_preview_module    {nullptr};
    Platform::AbstractRenderCanvas* m_canvas            {nullptr};
};

} // Slic3r::App

