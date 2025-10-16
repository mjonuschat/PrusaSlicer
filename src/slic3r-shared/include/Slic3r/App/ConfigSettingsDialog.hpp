///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractSettingsDialog.hpp"
#include "Slic3r/App/Config/ObservableCategorizer.hpp"
#include "Slic3r/App/Config/CategoryPageTransformer.hpp"

namespace Slic3r::Biz {
class ConfigBoxInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigSubcategoryListView;
class Navigator;

class ConfigSettingsDialog : public Yoga::AbstractSettingsDialog
{
public:
    ConfigSettingsDialog(
        Biz::ProjectInteractor& project_interactor,
        Navigator& navigator,
        const std::string& name = {}
    );

    virtual void navigate_to_item(const Domain::ConfigItem* config_item);
    virtual void clear_navigation();

protected:
    void remove_tab(size_t index) override;

private:
    void on_about_to_close() override;

protected:
    struct ConfigTab
    {
        ConfigTab(
            Biz::ConfigBoxInteractor* cbi,
            Tab* tab,
            Biz::ProjectInteractor& project_interactor
        );

        void navigate_to_item(const Domain::ConfigItem* config_item);
        void clear_navigation();

        Biz::ConfigBoxInteractor* cbi{nullptr};
        Tab* tab{nullptr};
        Biz::ProjectInteractor& project_interactor;
        Biz::UnsharedPointer<ObservableCategorizer> observable_categorizer;
        Biz::UnsharedPointer<CategoryPageTransformer> category_page_transformer;
    };

    using ConfigTabPtr = std::unique_ptr<ConfigTab>;
    using ConfigTabs   = std::vector<ConfigTabPtr>;

    ConfigTabs m_config_tabs;

    Biz::ProjectInteractor& m_project_interactor;
    Navigator& m_navigator;
};

} // namespace Slic3r::App
