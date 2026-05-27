///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/MaterialSettingsDialog.hpp"

#include "Slic3r/Biz/OverridableConfigBoxInteractor.hpp"
#include "Slic3r/Biz/OverridableConfigBoxObservableList.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Config/OverridableSubcategoryListView.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/CurrentPresetLabel.hpp"
#include "Slic3r/App/Config/CategoryUtils.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/MaterialSelectionDialog.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/IDialogManager.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;
using namespace Slic3r::Biz;

namespace Slic3r::App {

MaterialSettingsDialog::MaterialSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator,
    MaterialSelectionDialog* material_selection_dialog
) :
    AbstractSettingsDialog({}, "MaterialSettingsDialog"),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_material_selection_dialog(material_selection_dialog),
    m_material_cbi_list(m_project_interactor.preset_interactor().material_cbi_list())
{
    m_material_cbi_list.add_listener<Biz::IListObserver<Biz::OverridableConfigBoxInteractor>>(this);

    m_footer->set_gap(5.f);

    m_current_preset_label = m_footer->emplace_back<CurrentPresetLabel>(
        m_project_interactor.preset_interactor().material_presets()
    );
    m_current_preset_label->set_align(Align{AlignH::Center, AlignV::Center});

    m_footer->emplace_back<Separator>(Orientation::Vertical);

    add_footer_button(_u8L("Compare"), Render::Icon::Compare)->callbacks().action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(
            m_project_interactor.preset_interactor(),
            Domain::Preset::PresetKind::FdmMaterial
        );
    };

    add_footer_button(_u8L("Save preset"))->callbacks().action = [&]
    {
        m_project_interactor.preset_interactor().save_user_preset(
            Domain::Preset::PresetKind::FdmMaterial,
            current_tab_index()
        );
    };

    on_reset();
}

MaterialSettingsDialog::~MaterialSettingsDialog()
{
    m_material_cbi_list.remove_listener<Biz::IListObserver<Biz::OverridableConfigBoxInteractor>>(
        this
    );
}

void MaterialSettingsDialog::navigate_to_item(const Domain::ConfigItem* config_item)
{
    m_config_tabs.front()->navigate_to_item(config_item);
}

void MaterialSettingsDialog::clear_navigation()
{
    m_config_tabs.front()->clear_navigation();
}

void MaterialSettingsDialog::on_reset()
{
    Tabs::const_iterator current_tab_it = std::find_if(
        m_tabs.cbegin(),
        m_tabs.cend(),
        [this](const std::unique_ptr<Tab>& tab) { return tab.get() == m_current_tab; }
    );

    std::optional<size_t> current_index;
    if (current_tab_it != m_tabs.cend()) {
        current_index = std::distance(m_tabs.cbegin(), current_tab_it);
    }

    while (!m_config_tabs.empty()) {
        remove_tab(0);
    }

    for (size_t material_cbi_index = 0; material_cbi_index < m_material_cbi_list.size();
         ++material_cbi_index)
    {
        Biz::OverridableConfigBoxInteractor& cbi = const_cast<Biz::OverridableConfigBoxInteractor&>(
            m_material_cbi_list.at(material_cbi_index)
        );

        std::string tab_name = m_project_interactor.selected_config_container().print_technology()
                == Domain::PrinterTechnology::FFF ?
            _u8L("Filament") :
            _u8L("Material");

        Tab* tab = append_tab(fmt::format("{} {}", tab_name, material_cbi_index + 1));
        m_config_tabs.emplace_back(
            std::make_unique<ConfigTab>(cbi, *tab, m_project_interactor, material_cbi_index)
        );
    }

    if (current_index.has_value()) {
        set_current_tab(std::min(current_index.value(), m_tabs.size() - 1));
    }
}

void MaterialSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(m_material_selection_dialog);
}

void MaterialSettingsDialog::remove_tab(size_t index)
{
    AbstractSettingsDialog::remove_tab(index);
    m_config_tabs.erase(m_config_tabs.begin() + index);
}

void MaterialSettingsDialog::on_tab_selected(int current_index)
{
    AbstractSettingsDialog::on_tab_selected(current_index);

    if (m_current_tab && current_index < static_cast<int>(m_material_cbi_list.size())) {
        m_current_preset_label->set_current_list(current_index);
    }
}

void MaterialSettingsDialog::on_about_to_close() {
    clear_navigation();
}

MaterialSettingsDialog::ConfigTab::ConfigTab(
    Biz::OverridableConfigBoxInteractor& cbi,
    Tab& tab,
    Biz::ProjectInteractor& project_interactor,
    size_t cbi_index
) :
    cbi(cbi),
    tab(tab),
    project_interactor(project_interactor),
    cbi_index(cbi_index),
    categorizer(std::make_shared<Categorizer>()),
    category_page_transformer(std::make_shared<OverridableCategoryPageTransformer>())
{
    categorizer->set_filter_fn(
        [](const Biz::OverrideItem& item)
        { return item.config_item->def().category != Domain::ConfigItemDef::Category::Hidden; }
    );
    categorizer->set_group_by_fn(
        [](const Biz::OverrideItem& item,
           std::unordered_set<Domain::ConfigItemDef::Category>& seen_keys) -> bool
        {
            const Domain::ConfigItemDef::Category category = item.is_override() ?
                Domain::ConfigItemDef::Category::Filament_Overrides :
                item.config_item->def().category;
            DEBUG_ASSERT(
                category != Domain::ConfigItemDef::Category::Unknown,
                "ConfigItemDef cannot have unkown category, please fill it."
            );

            if (seen_keys.contains(category)) {
                return true;
            } else {
                seen_keys.insert(category);
                return false;
            }
        }
    );
    categorizer->set_sort_fn(
        [](const Biz::OverrideItem& lhs, const Biz::OverrideItem& rhs)
        { return lhs.config_item->def().category < rhs.config_item->def().category; }
    );
    categorizer->set_source_model(cbi.config_box_overridable_list());

    category_page_transformer->set_transform_fn(
        [this](const Biz::OverrideItem& data, size_t index)
        {
            const Domain::ConfigItemDef::Category category = data.is_override() ?
                Domain::ConfigItemDef::Category::Filament_Overrides :
                data.config_item->def().category;

            Domain::PrinterTechnology pt =
                this->project_interactor.selected_config_container().print_technology();
            Render::Icon icon = CategoryUtils::category_render_icon(category, pt);

            return PageEntry{
                Biz::_u8(Domain::ConfigItemDef::translate_category(category, pt)),
                icon
            };
        }
    );

    category_page_transformer->set_source_model(categorizer.get());

    using OverridableCategoryListViewFactory = ViewFactory<
        OverridableSubcategoryListView,
        Biz::OverrideItem,
        Biz::IConfigBoxSetter&,
        Biz::OverridableConfigBoxInteractor&,
        size_t>;
    using OverridableCategoryListView = ListView<
        OverridableSubcategoryListView,
        Biz::OverrideItem,
        OverridableCategoryListViewFactory,
        StackLayout>;

    std::unique_ptr<OverridableCategoryListView> category_list_view =
        std::make_unique<OverridableCategoryListView>(OverridableCategoryListViewFactory{
            project_interactor.preset_interactor(),
            cbi,
            cbi_index
        });

    category_list_view->set_source_list(categorizer.get());

    tab.replace_stack_layout(std::move(category_list_view));

    tab.page_list_view->set_source_list(category_page_transformer.get());
}

void MaterialSettingsDialog::ConfigTab::navigate_to_item(const Domain::ConfigItem* config_item)
{
    clear_navigation();

    const bool is_override = config_item->def().location != config_item->location();

    const Domain::ConfigItemDef::Category category = is_override ? Domain::ConfigItemDef::Category::Filament_Overrides : config_item->def().category;
    for (size_t category_index = 0; category_index < categorizer->size(); ++category_index) {
        if (categorizer->at(category_index).config_item->def().category == category) {
            tab.page_list_view->item_at(category_index)->callbacks().action();
            dynamic_cast<OverridableSubcategoryListView*>(
                tab.pages_stack_layout->get_item(category_index)
            )
                ->navigate_to_item(config_item);
            break;
        }
    }
}

void MaterialSettingsDialog::ConfigTab::clear_navigation()
{
    for (size_t index = 0; index < tab.pages_stack_layout->object_count(); ++index) {
        dynamic_cast<OverridableSubcategoryListView*>(tab.pages_stack_layout->get_item(index))
            ->clear_navigation();
    }
}

} // namespace Slic3r::App
