#include "TestFrame.hpp"
#include "IPanel.hpp"

#include <imgui.h>

namespace Slic3r::App::View {

struct TestPanel : IPanel
{
    void render_imgui() override
    {
        ImGui::Begin("##TestPanel");
        ImGui::Text("Hello from Test Panel");
        ImGui::SliderFloat("Value", &value, 0, 10);

        ImGui::InputText("Text", str, str_capacity);

        if (ImGui::Button("OK")) {

        }

        ImGui::End();
    }

    float value {0};
    constexpr static size_t str_capacity = 64;
    char str[str_capacity] {0};
};

TestFrame::TestFrame() {
    m_views.emplace_back(std::make_unique<TestPanel>());
}

} // namespace Slic3r::App::View
