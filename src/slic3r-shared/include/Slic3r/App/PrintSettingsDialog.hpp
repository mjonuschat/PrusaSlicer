///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ConfigSettingsDialog.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class Navigator;

class TmpSettingsDialog :
    public Yoga::Dialog,
    public Biz::IListObserver<Biz::ConfigBoxInteractor>,
    public Biz::Scene::ISceneBedInstanceChangedListener,
    public Biz::ISelectedBedInstancesChangedListener
{
public:
    explicit TmpSettingsDialog(Biz::ProjectInteractor& project_interactor);

    void on_reset() override;

    virtual void on_bed_instance_extruder_candidates_changed(
        Domain::SelectionId project_id,
        Domain::BedRef instance,
        const std::vector<unsigned>& extruder_candidates
    ) override;

    virtual void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::BedSelection& bed_selection
    ) override;

private:
    Biz::ProjectInteractor& m_project_interactor;
    Biz::CBIObservableList& m_tool_cbi_list;
    Biz::ConfigBoxInteractor& m_print_cbi;
    Yoga::ScrollArea* m_items;
    std::vector<unsigned> m_extruder_candidates;
    Domain::SelectionId m_selected_bed;

    void display_items(Yoga::Item& parent);
};

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
