#include "TestRenderModule.hpp"
#include "BaseRenderModule.hpp"
#include "TestView.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::View {

TestRenderModule::TestRenderModule() {
    m_views.emplace_back(std::make_unique<TestView>());
}

}
// namespace Slic3r::App::View
