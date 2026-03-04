#pragma once

#include <vector>

#include "Slic3r/Biz/IColorsChangedListener.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/SelectionId.hpp"

namespace Slic3r::Biz {
class ProjectSettingsInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Plater {

/**
 * @brief Always-visible in-scene ImGui debug panel for filament slot colors.
 *
 * Implements IColorsChangedListener so it stays in sync with the interactor.
 * Shows a color picker per slot and writes back via set_color_from_user().
 *
 * Usage:
 *   1. Register as a listener on ProjectSettingsInteractor.
 *   2. Call render_imgui() every frame from the scene's ImGui render pass.
 */
class ColorsDebugDialog : public Biz::IColorsChangedListener
{
public:
    /**
     * @brief Render the debug panel.
     *
     * @param interactor        The interactor to read from / write to.
     * @param config_container_id  The currently selected config container.
     */
    void render_imgui(
        Biz::ProjectSettingsInteractor& interactor,
        Domain::SelectionId config_container_id
    );

    /**
     * @name IColorsChangedListener
     * @{
     */
    void on_colors_changed(
        Domain::SelectionId config_container_id,
        const std::vector<Domain::ColorRGB>& colors
    ) override;
    /** @} */

private:
    Domain::SelectionId m_container_id{Domain::INVALID_ID};
    std::vector<Domain::ColorRGB> m_colors;
};

} // namespace Slic3r::App::Plater
