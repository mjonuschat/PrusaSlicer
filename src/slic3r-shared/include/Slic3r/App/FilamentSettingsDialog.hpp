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

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class FilamentSettingsDialog :
    public Yoga::AbstractSettingsDialog,
    public Biz::IListObserver<Biz::ConfigBoxInteractor>
{
public:
    explicit FilamentSettingsDialog(Biz::ProjectInteractor& project_interactor);
    ~FilamentSettingsDialog();

    void on_reset() override;

private:
    struct FilamentTab
    {
        FilamentTab(
            Biz::ConfigBoxInteractor* cbi,
            Tab* tab,
            Biz::Preset::PresetInteractor& preset_interactor
        );

        Biz::ConfigBoxInteractor* cbi{nullptr};
        Tab* tab{nullptr};
        Biz::UnsharedPointer<ObservableCategorizer> observable_categorizer;
        Biz::UnsharedPointer<CategoryPageTransformer> category_page_transformer;
    };

    Biz::ProjectInteractor& m_project_interactor;
    Biz::CBIObservableList& m_material_cbi_list;

    std::vector<std::unique_ptr<FilamentTab>> m_filaments;
};

} // namespace Slic3r::App
