///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableCategorizer.hpp"
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"
#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class Navigator;
class MaterialSelectionDialog;

class MaterialSettingsDialog :
    public Yoga::AbstractSettingsDialog,
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
    struct MaterialTab
    {
        MaterialTab(Biz::ConfigBoxInteractor* cbi, Tab* tab, Biz::ProjectInteractor& project_interactor);

        Biz::ConfigBoxInteractor* cbi{nullptr};
        Tab* tab{nullptr};
        Biz::UnsharedPointer<ObservableCategorizer> observable_categorizer;
        Biz::UnsharedPointer<CategoryPageTransformer> category_page_transformer;
    };

    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
    MaterialSelectionDialog* m_material_selection_dialog{nullptr};
    Biz::CBIObservableList& m_material_cbi_list;

    std::vector<std::unique_ptr<MaterialTab>> m_materials;
};

} // namespace Slic3r::App
