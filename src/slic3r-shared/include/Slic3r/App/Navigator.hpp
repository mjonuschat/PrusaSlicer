#pragma once
#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

namespace Plater {
class PlaterRenderModule;
} // namespace Plater

namespace Preview {
class PreviewRenderModule;
} // namespace Preview

namespace Platform {
class AbstractRenderCanvas;
} // namespace Platform

namespace Render {
enum class ModuleType
{
    Undef,
    Plater,
    Preview,
};
} // namespace Render

class Navigator : public Biz::ISelectedProjectChangedListener
{
public:
    ~Navigator();

    void on_init(
        Plater::PlaterRenderModule& plater_module,
        Preview::PreviewRenderModule& preview_module,
        Platform::AbstractRenderCanvas& canvas,
        Biz::ProjectInteractor* project_interactor
    );

    void set_render_module_type(Render::ModuleType type);

    void on_selected_project_changed(size_t index) override;

private:
    struct ProjectContext
    {
        Render::ModuleType type{Render::ModuleType::Plater};
    };

    using ProjectContexts    = Biz::ProjectScoped<ProjectContext>;
    using ProjectContextsPtr = std::unique_ptr<ProjectContexts>;

    ProjectContextsPtr m_project_contexts;

    Plater::PlaterRenderModule* m_plater_module{nullptr};
    Preview::PreviewRenderModule* m_preview_module{nullptr};
    Platform::AbstractRenderCanvas* m_canvas{nullptr};
};

} // namespace Slic3r::App
