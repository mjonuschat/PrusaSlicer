#include "Slic3r/App/Navigator.hpp"
#include <Slic3r/App/Plater/PlaterRenderModule.hpp>
#include <Slic3r/App/Preview/PreviewRenderModule.hpp>
#include "Slic3r/App/Platform/AbstractRenderCanvas.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

namespace Slic3r::App {

Navigator::~Navigator() {}

void Navigator::on_init(
    Plater::PlaterRenderModule& plater_module,
    Preview::PreviewRenderModule& preview_module,
    Platform::AbstractRenderCanvas& canvas,
    Biz::ProjectInteractor* project_interactor
)
{
    m_plater_module  = &plater_module;
    m_preview_module = &preview_module;
    m_plater_module->set_navigator(this);
    m_preview_module->set_navigator(this);
    m_canvas           = &canvas;
    m_project_contexts = std::make_unique<ProjectContexts>(*project_interactor);

    project_interactor->add_listener<ISelectedProjectChangedListener>(this);
}

void Navigator::set_render_module_type(Render::ModuleType type)
{
    m_project_contexts->selected().type = type;

    if (type == Render::ModuleType::Plater) {
        m_canvas->set_next_render_module(m_plater_module);
    } else if (type == Render::ModuleType::Preview) {
        m_canvas->set_next_render_module(m_preview_module);
    }
}

void Navigator::on_selected_project_changed(size_t index)
{
    set_render_module_type(m_project_contexts->selected().type);
}

} // namespace Slic3r::App
