#include "Slic3r/App/Navigator.hpp"
#include <Slic3r/App/Plater/PlaterRenderModule.hpp>
#include <Slic3r/App/Preview/PreviewRenderModule.hpp>
#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"

namespace Slic3r::App {

Navigator::~Navigator()
{
    m_plater_module->remove_type_changed_listener(this);
    m_preview_module->remove_type_changed_listener(this);
}

void Navigator::on_init(Plater::PlaterRenderModule& plater_module,
                        Preview::PreviewRenderModule& preview_module,
                        Platform::AbstractRenderCanvas& canvas)
{
    m_plater_module = &plater_module;
    m_preview_module = &preview_module;
    m_canvas = &canvas;

    m_plater_module->add_type_changed_listener(this);
    m_preview_module->add_type_changed_listener(this);
}

void Navigator::on_render_module_changed(Render::ModuleType type)
{
    if (type==Render::ModuleType::Plater)
        m_canvas->set_next_render_module(m_plater_module);
    else if (type==Render::ModuleType::Preview)
        m_canvas->set_next_render_module(m_preview_module);
};

} // Slic3r::App

