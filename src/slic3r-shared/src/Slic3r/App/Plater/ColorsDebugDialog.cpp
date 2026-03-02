#include "Slic3r/App/Plater/ColorsDebugDialog.hpp"

#include <imgui/imgui.h>

#include "Slic3r/Biz/Algorithms/Color.hpp"
#include "Slic3r/Biz/ProjectSettingsInteractor.hpp"

namespace Slic3r::App::Plater {

void ColorsDebugDialog::on_colors_changed(
    Domain::SelectionId config_container_id,
    const std::vector<std::string>& colors
)
{
    m_container_id = config_container_id;
    m_colors = colors;
}

void ColorsDebugDialog::render_imgui(
    Biz::ProjectSettingsInteractor& interactor,
    Domain::SelectionId config_container_id
)
{
    // Refresh cache if the active container changed.
    if (config_container_id != m_container_id) {
        m_container_id = config_container_id;
        m_colors = interactor.get_colors(config_container_id);
    }

    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
    if (!ImGui::Begin("Filament slot colors", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    if (m_colors.empty()) {
        ImGui::Text("No slots");
    } else {
        for (int slot = 0; slot < static_cast<int>(m_colors.size()); ++slot) {
            Domain::ColorRGB clr;
            Biz::Algorithms::Color::decode_color(m_colors[slot], clr);
            float col[3] = {clr.r(), clr.g(), clr.b()};

            ImGui::PushID(slot);

            char label[32];
            std::snprintf(label, sizeof(label), "Slot %d", slot + 1);

            if (ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_NoInputs)) {
                const std::string new_hex = Biz::Algorithms::Color::encode_color(Domain::ColorRGB{col[0], col[1], col[2]});
                interactor.set_color_from_user(config_container_id, slot, new_hex);
            }

            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                interactor.set_color_from_user(config_container_id, slot, "");
            }

            ImGui::PopID();
        }
    }

    ImGui::End();
}

} // namespace Slic3r::App::Plater
