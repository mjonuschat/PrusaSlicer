///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/MaterialSelectionDialog.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/InputText.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Search.hpp"
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/MaterialSettingsDialog.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include <boost/locale.hpp>

using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

namespace Slic3r::App {

MaterialSelectionDialog::MaterialSelectionDialog(
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator
) :
    Yoga::Dialog({"Material"}, "MaterialSelectionDialog"),
    m_project_interactor(project_interactor),
    m_navigator(navigator),
    m_material_presets(m_project_interactor.preset_interactor().material_presets()),
    m_material_filter(std::make_shared<Biz::ObservableListSortFilter<Biz::Preset::PresetItem>>())
{
    m_material_settings_dialog =
        content_item()->emplace_back<MaterialSettingsDialog>(project_interactor, m_navigator, this);
    m_material_presets.add_listener<Biz::IListObserver<Biz::Preset::PresetItemObservableList>>(
        this
    );

    content_item()->set_width(350);

    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(5);

    Item* search_row = content()->emplace_back<Item>();
    search_row->set_gap(5);
    Icon* icon = search_row->emplace_back<Icon>(Render::Icon::Search);
    icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
    m_input_text_search = search_row->emplace_back<InputText>();
    m_input_text_search->set_hint(_u8L("Search..."));
    m_input_text_search->set_flex_grow(1);

    m_material_filter->set_filter_fn(
        [this](const Biz::Preset::PresetItem& data) -> bool
        {
            const std::string& search_text = m_input_text_search->text();
            if (search_text.empty()) {
                return true;
            }

            return find_locale_aware(data.name, search_text);
        }
    );

    m_input_text_search->callbacks().text_changed = [this]() { m_material_filter->invalidate(); };

    content()->emplace_back<Separator>(Orientation::Horizontal);

    ScrollArea* scroll_area = content()->emplace_back<ScrollArea>("Foobar");
    scroll_area->set_orientation(Orientation::Vertical);
    scroll_area->set_min_size({0, 200});
    scroll_area->set_max_size({YGUndefined, 200});

    m_selection_row_list_view = scroll_area->emplace_back<SelectionRowListView>(
        SelectionRowListViewFactory{m_material_index, m_project_interactor.preset_interactor()}
    );
    m_selection_row_list_view->set_orientation(Orientation::Vertical);
    m_selection_row_list_view->set_gap(5);
    m_selection_row_list_view->set_padding(Paddings(0, 0, 10, 0));
    m_selection_row_list_view->set_source_list(m_material_filter.get());

    content()->emplace_back<Separator>(Orientation::Horizontal);

    m_advanced_button = content()->emplace_back<LayoutButton>(_u8L("Advanced settings"));
    m_advanced_button->set_checkable(true);
    m_advanced_button->callbacks().action = [this]
    {
        if (m_material_settings_dialog->opened()) {
            m_navigator.set_opened_dialog(this);
        } else {
            m_navigator.set_opened_dialog(m_material_settings_dialog);
        }
    };

    // Material Settings Dialog setup
    m_material_settings_dialog->attach_to_item(content_item(), Position::Left);

    m_material_settings_dialog->dialog_callbacks().tab_selected = [this](size_t current_index)
    {
        if (m_callbacks.advanced_settings_tab_opened) {
            m_callbacks.advanced_settings_tab_opened(current_index);
        }
    };
    m_material_settings_dialog->callbacks().closed = [this]()
    { m_advanced_button->set_checked(false); };
}

MaterialSelectionDialog::~MaterialSelectionDialog()
{
    m_material_presets.remove_listener<Biz::IListObserver<Biz::Preset::PresetItemObservableList>>(
        this
    );
}

MaterialSelectionDialog::Callbacks& MaterialSelectionDialog::material_selection_callbacks()
{
    return m_callbacks;
}

void MaterialSelectionDialog::set_material_index(size_t material_index)
{
    if (m_material_index == material_index) {
        return;
    }

    m_material_index = material_index;

    update_preset_list();
}

void MaterialSelectionDialog::on_list_selection_changed(Domain::SelectionId new_selection)
{
    for (size_t index = 0; index < m_material_filter->size(); ++index) {
        AbstractButton* button =
            dynamic_cast<AbstractButton*>(m_selection_row_list_view->item_at(index));
        ASSERT(button);
        button->set_checked(index == new_selection);
    }
}

void MaterialSelectionDialog::on_will_be_reset()
{
    if (m_preset_list) {
        m_preset_list->remove_listener<Biz::IListSelectionChangedListener>(this);
        m_preset_list = nullptr;
    }

    m_material_filter->set_source_model(nullptr);
}

void MaterialSelectionDialog::on_reset()
{
    m_material_index = m_material_presets.size() ?
        std::clamp(m_material_index, std::size_t{0}, m_material_presets.size() - 1) :
        Domain::INVALID_ID;

    update_preset_list();
}

void MaterialSelectionDialog::close_action()
{
    m_navigator.set_opened_dialog(nullptr);
}

void MaterialSelectionDialog::update_preset_list()
{
    if (m_preset_list) {
        m_preset_list->remove_listener<Biz::IListSelectionChangedListener>(this);
        m_preset_list = nullptr;
    }

    if (m_material_index == Domain::INVALID_ID) {
        return;
    }

    m_preset_list = &m_material_presets.at(m_material_index);
    m_preset_list->add_listener<Biz::IListSelectionChangedListener>(this);

    m_material_filter->set_source_model(&m_preset_list->items());
    on_list_selection_changed(m_preset_list->selected_index());
}

MaterialSettingsDialog& MaterialSelectionDialog::material_settings_dialog()
{
    return *m_material_settings_dialog;
}

} // namespace Slic3r::App
