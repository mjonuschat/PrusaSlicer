///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ConfigSettingsDialog.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class Navigator;

class PrintSettingsDialog :
    public ConfigSettingsDialog,
    public Biz::IListObserver<Biz::ConfigBoxInteractor>
{
public:
    explicit PrintSettingsDialog(Biz::ProjectInteractor& project_interactor, Navigator& navigator);
    ~PrintSettingsDialog();

    void on_reset() override;

    void navigate_to_item(const Domain::ConfigItem *config_item) override;
    void clear_navigation() override;

protected:
    void close_action() override;

private:
    Biz::CBIObservableList& m_tool_cbi_list;
};

} // namespace Slic3r::App
