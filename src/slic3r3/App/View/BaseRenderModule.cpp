#include "BaseRenderModule.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::View {

void BaseRenderModule::render_imgui() {
    for (auto &view : m_views) {
        view->render_imgui();
    }
}

void BaseRenderModule::on_mouse_event(const MouseEvent&)
{
    
}


void BaseRenderModule:: on_keyboard_event(const KeyEvent&)
{

}


} // namespace Slic3r::App::View
