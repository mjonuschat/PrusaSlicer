///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableCategorizer.hpp"
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"

namespace Slic3r::Biz {
class ProjectInteractor;
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class FilamentSettingsDialog : public Yoga::AbstractSettingsDialog
{
    struct FilamentTab
    {
        Biz::ConfigBoxInteractor& cbi;
        Tab* tab{nullptr};
        ObservableCategorizer observable_categorizer;
        CategoryPageTransformer category_page_transformer;
    };

public:
    explicit FilamentSettingsDialog(Biz::ProjectInteractor& project_interactor);

private:
    Biz::ProjectInteractor& m_project_interactor;

    std::vector<FilamentTab> m_filaments;
};

} // namespace Slic3r::App
