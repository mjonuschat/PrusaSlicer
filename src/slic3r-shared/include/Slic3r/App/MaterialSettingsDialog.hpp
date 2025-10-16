///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ConfigSettingsDialog.hpp"
#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class Navigator;
class MaterialSelectionDialog;
class ConfigSubcategoryListView;

class MaterialSettingsDialog :
    public ConfigSettingsDialog,
    public Biz::IListObserver<Biz::ConfigBoxInteractor>
{
public:
    explicit MaterialSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        MaterialSelectionDialog* material_selection_dialog
    );
    ~MaterialSettingsDialog();

    void on_reset() override;

protected:
    void close_action() override;

private:
    MaterialSelectionDialog* m_material_selection_dialog{nullptr};
    Biz::CBIObservableList& m_material_cbi_list;
};

} // namespace Slic3r::App
