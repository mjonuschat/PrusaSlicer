///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableCategorizer.hpp"
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class PrintSettingsDialog :
    public Yoga::AbstractSettingsDialog,
    public Biz::IListObserver<Biz::ConfigBoxInteractor>
{
public:
    explicit PrintSettingsDialog(Biz::ProjectInteractor& project_interactor);
    ~PrintSettingsDialog();

    void on_reset() override;

private:
    struct PrintSettingsTab
    {
        PrintSettingsTab(
            Biz::ConfigBoxInteractor* cbi,
            Yoga::AbstractSettingsDialog::Tab* tab,
            Biz::Preset::PresetInteractor& preset_interactor
        );

        Biz::ConfigBoxInteractor* cbi{nullptr};
        Yoga::AbstractSettingsDialog::Tab* tab{nullptr};
        ObservableCategorizer observable_categorizer;
        CategoryPageTransformer category_page_transformer;
    };

    Biz::ProjectInteractor& m_project_interactor;
    Biz::CBIObservableList& m_tool_cbi_list;

    std::vector<std::unique_ptr<PrintSettingsTab>> m_tabs;
};

} // namespace Slic3r::App
