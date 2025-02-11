#include "Slic3r/App/Preview/PreviewRenderModule.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"

namespace Slic3r::App::Preview {

void PreviewRenderModule::render_scene()
{
    m_device->load_state();
    auto cmd_buffer = m_device->create_command_buffer();

    cmd_buffer->set_viewport(Render::Rect::from(0, 0, m_screen_info));
    cmd_buffer->set_clear_values({0.61f, 0.61f, 0.61f, 1.00f});
    cmd_buffer->clear_buffers(true, true);

    m_scene_presenter->render_scene(*cmd_buffer);

    cmd_buffer->submit();
}

void PreviewRenderModule::render_imgui()
{
}

void PreviewRenderModule::on_init(Render::Device& device)
{
    AbstractRenderModule::on_init(device);
    m_scene_presenter =
        std::make_unique<Plater::ScenePresenter>(m_workbench, m_project_interactor, *m_device);
    m_project_interactor.add_selected_project_changed_listener(m_scene_presenter.get());
    m_project_interactor.scene_interactor().add_scene_changed_listener(m_scene_presenter.get());

    try {
        App::LibvgcodeWrapper::WrapperSettings settings;
        if (!m_viewer.init(settings)) {
        }
    }
    catch (const std::exception& e) {
    }
}

} // namespace Slic3r::App::Preview
