///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrintSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/PrintToolConfigObservableList.hpp"

#include "Slic3r/App/Config/CategoryUtils.hpp"
#include "Slic3r/App/Navigator.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Yoga/StackLayout.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;
using namespace Slic3r::Biz;

namespace Slic3r::App {

PrintSettingsDialog::PrintSettingsDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator
) :
    Dialog("PrintSettingsDialog"),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_selected_bed_changed_scope(m_project_interactor.scene_interactor(), *this),
    m_scene_bed_instance_changed_scope(m_project_interactor.scene_interactor(), *this),
    m_preset_changed_listener_scope(m_project_interactor.preset_interactor(), *this),
    m_tool_print_categorizer(std::make_shared<ToolPrintCategorizer>()),
    m_tool_print_transformer(std::make_shared<ToolPrintMenuTransformer>()),
    m_extruder_menu_transformer(std::make_shared<ExtruderMenuTransformer>())
{
    content_item()->set_width(700);
    content_item()->set_height(700);

    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(0);
    content()->set_flex_grow(1);
    // Only preserve 1px bottom padding
    content()->set_padding(Paddings(0, 0, 0, 1));

    append_tab(Biz::_u8L("Print"));

    Item* center_row = content()->emplace_back<Item>();
    center_row->set_orientation(Orientation::Horizontal);
    center_row->set_flex_grow(1);

    m_tool_print_categorizer->set_filter_fn(
        [](const Biz::PrintToolItem& item)
        { return item.print_item->def().category != Domain::ConfigItemDef::Category::Hidden; }
    );
    m_tool_print_categorizer->set_group_by_fn(
        [](const Biz::PrintToolItem& item,
           std::unordered_set<Domain::ConfigItemDef::Category>& seen_keys) -> bool
        {
            DEBUG_ASSERT(
                item.print_item->def().category != Domain::ConfigItemDef::Category::Unknown,
                "ConfigItemDef cannot have unkown category, please fill it."
            );

            if (seen_keys.contains(item.print_item->def().category)) {
                return true;
            } else {
                seen_keys.insert(item.print_item->def().category);
                return false;
            }
        }
    );
    m_tool_print_categorizer->set_sort_fn(
        [](const Biz::PrintToolItem& lhs, const Biz::PrintToolItem& rhs) -> int
        {
            return static_cast<uint8_t>(lhs.print_item->def().category)
                < static_cast<uint8_t>(rhs.print_item->def().category);
        }
    );

    m_tool_print_transformer->set_transform_fn(
        [this](const Biz::PrintToolItem& data, size_t index) -> PageEntry
        {
            const Domain::ConfigItemDef::Category category = data.print_item->def().category;

            Domain::PrinterTechnology pt =
                m_project_interactor.selected_config_container().print_technology();
            Render::Icon icon = CategoryUtils::category_render_icon(category, pt);

            return PageEntry{Domain::ConfigItemDef::translate_category(category, pt), icon};
        }
    );
    m_tool_print_transformer->set_source_model(m_tool_print_categorizer.get());

    // Create the ViewFactory explicitly:
    auto factory_category =
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>(
            [this](size_t index) { select_page_entry(index, true); }
        );
    auto factory_extruder =
        Yoga::ViewFactory<PageEntryButton, PageEntry, PageEntryButton::FnIndexClicked>(
            [this](size_t index) { select_page_entry(index, false); }
        );
    ScrollArea* left_column = center_row->emplace_back<ScrollArea>();
    left_column->set_padding(Paddings(0, 0, 8, 0));
    left_column->set_orientation(Orientation::Vertical);
    left_column->set_gap(5);

    m_category_page_list_view =
        left_column->emplace_back<PageListView>(std::move(factory_category));
    m_category_page_list_view->set_object_name("CategoryPageListView");
    m_category_page_list_view->set_orientation(Orientation::Vertical);
    m_category_page_list_view->set_min_size({125, 0});
    m_category_page_list_view->set_flex_shrink(0);
    m_category_page_list_view->set_source_list(m_tool_print_transformer.get());
    m_category_page_list_view->set_flex_grow(1);

    m_tool_print_categorizer->set_source_model(
        m_project_interactor.preset_interactor().print_tool_cbi().observable_list()
    );

    m_extruder_menu_transformer->set_transform_fn(
        [](const Biz::Preset::ToolConfigItemObservableList& data, size_t index) -> PageEntry
        {
            return PageEntry{
                Biz::_u8L("Extruder") + " " + std::to_string(index + 1),
                Render::Icon::Funnel
            };
        }
    );
    m_extruder_menu_transformer->set_source_model(
        &m_project_interactor.preset_interactor().tool_items()
    );

    Text* extruders_label =
        left_column->emplace_back<Text>(Biz::_u8L("Extruders"), Render::ImguiFontType::Bold);
    extruders_label->set_margin({5, 0});
    extruders_label->set_flex_shrink(0);

    m_extruder_page_list_view =
        left_column->emplace_back<PageListView>(std::move(factory_extruder));
    m_extruder_page_list_view->set_object_name("ExtruderPageListView");
    m_extruder_page_list_view->set_orientation(Orientation::Vertical);
    m_extruder_page_list_view->set_min_size({125, 0});
    m_extruder_page_list_view->set_flex_shrink(0);
    m_extruder_page_list_view->set_flex_grow(1);
    m_extruder_page_list_view->set_source_list(m_extruder_menu_transformer.get());

    center_row->emplace_back<Separator>(Orientation::Vertical);

    m_content_stack_layout = center_row->emplace_back<StackLayout>();
    m_content_stack_layout->set_orientation(Orientation::Vertical);
    m_content_stack_layout->set_flex_grow(1);

    m_category_stack_list_view =
        m_content_stack_layout->emplace_back<ToolPrintCategoryListView>(ToolPrintCategoryFactory{
            m_project_interactor.preset_interactor().print_tool_cbi(),
            m_project_interactor.preset_interactor()
        });
    m_category_stack_list_view->set_orientation(Orientation::Vertical);
    m_category_stack_list_view->set_flex_grow(1);
    m_category_stack_list_view->set_source_list(m_tool_print_categorizer.get());

    m_metadata_stack_list_view = m_content_stack_layout->emplace_back<PrintMetadataListView>(
        m_project_interactor.preset_interactor()
    );
    m_metadata_stack_list_view->set_orientation(Orientation::Vertical);
    m_metadata_stack_list_view->set_flex_grow(1);
    m_metadata_stack_list_view->set_source_list(
        &m_project_interactor.preset_interactor().tool_items()
    );

    add_separator();

    m_footer = content()->emplace_back<Item>();
    m_footer->set_padding(10);
    m_footer->set_align_items(YGAlignCenter);
    m_footer->set_flex_shrink(0);
    m_footer->set_gap(5);

    m_bed_name = m_footer->emplace_back<Text>(std::string{});
    m_bed_name->set_flex_shrink(0);

    m_tool_label_list_view = m_footer->emplace_back<ToolLabelListView>();
    m_tool_label_list_view->set_orientation(Orientation::Horizontal);
    m_tool_label_list_view->set_gap(14);
    m_tool_label_list_view->set_source_list(this);
    m_tool_label_list_view->set_flex_shrink(0);

    Item* spacer = m_footer->emplace_back<Item>();
    spacer->set_flex_grow(1);

    LayoutButton* compare_button =
        m_footer->emplace_back<LayoutButton>(_u8("Compare"), Render::Icon::Compare);
    compare_button->callbacks().action = [this]
    {
        auto& dlg_manager = App::AppServices::instance().dialog_manager();
        dlg_manager.show_diff_dialog(
            m_project_interactor.preset_interactor(),
            Domain::Preset::PresetKind::FdmPrint
        );
    };
    compare_button->set_flex_shrink(0);
    compare_button->set_margin(Margins(0, 0, 10, 0));

    on_selected_bed_instances_changed(
        m_project_interactor.selected_project_id(),
        m_project_interactor.scene_interactor().bed_selection()
    );
    on_preset_selection_changed(0, 0, Biz::Preset::PresetItemType::PrinterPreset);
}

PrintSettingsDialog::~PrintSettingsDialog()
{
    m_tool_label_list_view->set_source_list(nullptr, true);
}

const bool& PrintSettingsDialog::at(size_t index) const
{
    return m_extruders.at(index);
}

size_t PrintSettingsDialog::size() const
{
    return m_extruders.size();
}

void PrintSettingsDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

void PrintSettingsDialog::select_page_entry(size_t index, bool category)
{
    for (size_t button_index = 0; button_index < m_category_page_list_view->object_count();
         ++button_index)
    {
        PageEntryButton* button =
            dynamic_cast<PageEntryButton*>(m_category_page_list_view->get_item(button_index));
        ASSERT(button);
        button->set_checked(category && index == button_index);
    }

    for (size_t button_index = 0; button_index < m_extruder_page_list_view->object_count();
         ++button_index)
    {
        PageEntryButton* button =
            dynamic_cast<PageEntryButton*>(m_extruder_page_list_view->get_item(button_index));
        ASSERT(button);
        button->set_checked(!category && index == button_index);
    }

    category ? m_category_stack_list_view->set_current_index(index) :
               m_metadata_stack_list_view->set_current_index(index);
    m_content_stack_layout->set_current_index(category ? 0 : 1);
}

void PrintSettingsDialog::update_extruder_candidates()
{
    if (m_extruders.empty()) {
        return;
    }

    std::fill(m_extruders.begin(), m_extruders.end(), false);

    const Domain::BedRef last_selected_bed =
        m_project_interactor.scene_interactor().bed_selection().last_selected_bed();

    const Domain::BedInstance* bed_instance =
        m_project_interactor.selected_project().find_bed_instance_by_id(
            last_selected_bed.instance_id
        );

    for (unsigned candidate : std::as_const(bed_instance->extruder_candidates)) {
        m_extruders.at(candidate) = true;
    }

    invoke_listeners<IListObserver<bool>>([&](IListObserver<bool>* l)
                                          { l->on_updated({0, m_extruders.size() - 1}); });
}

void PrintSettingsDialog::update_extruder_size()
{
    invoke_listeners<IListObserver<bool>>([&](IListObserver<bool>* l) { l->on_will_be_reset(); });

    m_extruders.resize(m_project_interactor.preset_interactor().tool_items().size());
    std::fill(m_extruders.begin(), m_extruders.end(), false);

    invoke_listeners<IListObserver<bool>>([&](IListObserver<bool>* l) { l->on_reset(); });
}

void PrintSettingsDialog::on_about_to_close()
{
    clear_navigation();
}

void PrintSettingsDialog::navigate_to_item(const Domain::ConfigItem* config_item)
{
    clear_navigation();

    const Domain::ConfigItemDef::Category category = config_item->def().category;
    for (size_t category_index = 0; category_index < m_tool_print_categorizer->size();
         ++category_index)
    {
        if (m_tool_print_categorizer->at(category_index).print_item->def().category == category) {
            m_content_stack_layout->set_current_index(0);
            m_category_page_list_view->item_at(category_index)->callbacks().action();
            dynamic_cast<PrintToolSubcategoryListView*>(
                m_category_stack_list_view->get_item(category_index)
            )
                ->navigate_to_item(config_item);
            break;
        }
    }
}

void PrintSettingsDialog::clear_navigation()
{
    for (size_t index = 0; index < m_category_stack_list_view->object_count(); ++index) {
        dynamic_cast<PrintToolSubcategoryListView*>(m_category_stack_list_view->get_item(index))
            ->clear_navigation();
    }
}

void PrintSettingsDialog::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::BedSelection& bed_selection
)
{
    if (project_id == Domain::INVALID_ID
        || m_project_interactor.selected_project_id() != project_id)
    {
        return;
    }

    Domain::BedInstance* bed_instance =
        m_project_interactor.selected_project().find_bed_instance_by_id(
            bed_selection.last_selected_bed().instance_id
        );

    ASSERT(bed_instance);
    m_bed_name->set_text(bed_instance->name());

    update_extruder_candidates();
}

void PrintSettingsDialog::on_bed_instance_extruder_candidates_changed(
    Domain::SelectionId project_id,
    Domain::BedRef instance,
    const std::vector<unsigned int>& extruder_candidates
)
{
    if (project_id == m_project_interactor.selected_project_id()
        && m_project_interactor.scene_interactor().bed_selection().last_selected_bed() == instance)
    {
        update_extruder_candidates();
    }
}

void PrintSettingsDialog::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (type == Biz::Preset::PresetItemType::PrinterPreset) {
        update_extruder_size();
    }
}

void PrintSettingsDialog::on_config_container_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id
)
{
    update_extruder_size();
}

} // namespace Slic3r::App
