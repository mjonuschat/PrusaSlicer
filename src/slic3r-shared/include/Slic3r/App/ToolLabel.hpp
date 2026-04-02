///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Biz/IColorsChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerScope.hpp"
#include "Slic3r/Biz/ProjectSettingsInteractor.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ToolLabel :
    public Biz::DataObserver<bool>,
    public Yoga::LayoutButton,
    public Biz::IColorsChangedListener
{
public:
    ToolLabel(
        size_t index,
        const bool& data,
        Biz::ProjectInteractor& project_interactor
    );

    void on_colors_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id,
        const std::vector<Domain::ColorRGB>& colors
    ) override;

protected:
    void on_index_update() override;
    void on_data_update() override;

    void update_markings();

private:
    Biz::ProjectInteractor& m_project_interactor;

    Biz::ListenerScope<Biz::IColorsChangedListener, Biz::ProjectSettingsInteractor, ToolLabel>
        m_colors_changed_listener_scope;

    Yoga::Text* m_label{nullptr};
};

} // namespace Slic3r::App
