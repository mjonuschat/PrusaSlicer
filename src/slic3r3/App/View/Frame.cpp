#include "Frame.hpp"
#include "IPanel.hpp"

namespace Slic3r::App::View {

void Frame::render_imgui() {
    for (auto& view : m_views) {
        view->render_imgui();
    }
}

void Frame::render_background(){}

}
