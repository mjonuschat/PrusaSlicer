///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/PrintToolRowItem.hpp"
#include "Slic3r/App/Config/FavoriteButton.hpp"

#include <Slic3r/Domain/Config.hpp>

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "Slic3r/App/Config/ConfigRowItem.hpp"
#include "Slic3r/App/Config/ConfigItemUtils.hpp"
#include "Slic3r/App/Config/PrintToolRowButton.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/AppConfigInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ExplanationView::ExplanationView(size_t index, const ExplanationPart& data) :
    Biz::DataObserver<ExplanationPart>(index, data),
    Text(std::string{})
{}

void ExplanationView::on_data_update()
{
    set_text(m_state->first);
    set_text_color(m_state->second);
}

PrintToolRowItem::PrintToolRowItem(
    size_t index,
    const Biz::PrintToolItem& data,
    Biz::PrintToolConfigBoxInteractor& cbi,
    Biz::IConfigBoxSetter& cbi_setter,
    Biz::ProjectInteractor& project_interactor,
    const PrintToolRowItemDisplayOptions& options
) :
    Biz::DataObserver<Biz::PrintToolItem>(index, data),
    m_cbi(cbi),
    m_cbi_setter(cbi_setter),
    m_project_interactor(project_interactor),
    m_colors_changed_listener_scope(project_interactor.project_settings_interactor(), *this),
    m_explanation_list_labels(std::make_shared<Biz::ObservableList<ExplanationPart>>()),
    m_explanation_list(std::make_shared<Biz::ObservableList<ExplanationPart>>())
{
    set_orientation(Orientation::Vertical);
    set_fill(IM_COL32_BLACK_TRANS);
    set_border_width(2);
    set_border_color(IM_COL32_BLACK_TRANS);

    m_header = emplace_back<Item>();
    m_header->set_orientation(Orientation::Horizontal);
    m_header->set_flex_grow(1.f);

    m_header->set_align_items(YGAlignFlexStart);
    m_header->set_gap(3.f);

    m_favorite_button = m_header->emplace_back<FavoriteButton>();
    m_favorite_button->set_visible(options.show_favorites);
    m_favorite_button->callbacks().action = [this]()
    { AppServices::instance().app_config_interactor().toggle_favorite_param(m_state->name); };

    on_data_update();
}

PrintToolRowItem::~PrintToolRowItem()
{
    if (m_tool_list_view) {
        m_tool_list_view->set_source_list(nullptr, true);
    }
}

void PrintToolRowItem::navigate_to_item(const Domain::ConfigItem* config_item)
{
    if (config_item && m_state->print_item->name() == config_item->name()) {
        set_border_color(m_theme->color_imgui(Platform::Color::AccentPrimary));
        if (m_main_button) {
            m_main_button->set_checked(true);
        }
    } else {
        set_border_color(IM_COL32_BLACK_TRANS);
    }
}

void PrintToolRowItem::clear_navigation()
{
    set_border_color(IM_COL32_BLACK_TRANS);
}

const ToolRowOverride& PrintToolRowItem::at(size_t index) const
{
    return m_tool_overrides.at(index);
}

size_t PrintToolRowItem::size() const
{
    return m_tool_overrides.size();
}

void PrintToolRowItem::on_colors_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    const std::vector<Domain::ColorRGB>& colors
)
{
    if (m_initialized_type == InitializedType::PrintTool) {
        update_explanation();
    }
}

void PrintToolRowItem::on_data_update()
{
    const bool need_multiple =
        m_state->shared_context.has_multiple_extruders && !m_state->tool_overrides.empty();
    const bool need_init = m_initialized_type == InitializedType::None
        || (m_initialized_type == InitializedType::PrintOnly && need_multiple)
        || (m_initialized_type == InitializedType::PrintTool && !need_multiple);

    if (need_init) {
        if (m_initialized_type != InitializedType::None) {
            clear();
        }
        initialize();
    }

    if (m_initialized_type == InitializedType::PrintTool) {
        m_main_button->update_data(m_state);
        const size_t new_tool_size = m_state->tool_overrides.size();
        if (m_tool_overrides.size() != new_tool_size) {
            invoke_listeners<Biz::IListObserver<ToolRowOverride>>(
                [](Biz::IListObserver<ToolRowOverride>* l) { l->on_will_be_reset(); }
            );
            m_tool_overrides.clear();
            size_t index = 0;
            std::transform(
                m_state->tool_overrides.cbegin(),
                m_state->tool_overrides.cend(),
                std::back_inserter(m_tool_overrides),
                [&](const Biz::PrintToolItem::ToolOverride& override) -> ToolRowOverride
                {
                    {
                        return ToolRowOverride{
                            override.first,
                            m_state->print_item,
                            override.second,
                            m_state->shared_context.extruder_candidates.contains(index++)
                        };
                    }
                }
            );
            invoke_listeners<Biz::IListObserver<ToolRowOverride>>(
                [](Biz::IListObserver<ToolRowOverride>* l) { l->on_reset(); }
            );
        } else if (new_tool_size) {
            ASSERT(m_tool_overrides.size() == new_tool_size);
            for (size_t index = 0; index < m_state->tool_overrides.size(); ++index) {
                ToolRowOverride& override_row = m_tool_overrides.at(index);
                const Biz::PrintToolItem::ToolOverride& override =
                    m_state->tool_overrides.at(index);
                override_row.override_item = override.first;
                override_row.print_item    = m_state->print_item;
                override_row.overriden     = override.second;
                override_row.extruder_candidate =
                    m_state->shared_context.extruder_candidates.contains(index);
            }
            invoke_listeners<Biz::IListObserver<ToolRowOverride>>(
                [new_tool_size](Biz::IListObserver<ToolRowOverride>* l)
                { l->on_updated({0, new_tool_size - 1}); }
            );
        }

        const ImColor text_color = m_theme->color_imgui(Platform::Color::Text);
        const ImColor disabled_color =
            m_theme->color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled);

        m_config_row_item->set_label_text_color(
            std::any_of(
                m_state->tool_overrides.cbegin(),
                m_state->tool_overrides.cend(),
                [&](const Biz::PrintToolItem::ToolOverride& override) { return !override.second; }
            ) ?
                text_color :
                disabled_color
        );

        update_explanation();
    }

    if (m_config_row_item) {
        m_config_row_item->set_state(*m_state->print_item);
    }

    m_favorite_button->set_checked(m_state->is_favorite);
}

void PrintToolRowItem::clear()
{
    if (m_initialized_type == InitializedType::PrintOnly) {
        m_header->remove(m_config_row_item);
        m_config_row_item = nullptr;
    } else if (m_initialized_type == InitializedType::PrintTool) {
        m_tool_list_view->set_source_list(nullptr, true);
        remove(m_content);
        m_header->remove(m_main_button);
        m_content     = nullptr;
        m_main_button = nullptr;
        m_tool_overrides.clear();
    }
    m_tool_list_view   = nullptr;
    m_initialized_type = InitializedType::None;
}

void PrintToolRowItem::initialize()
{
    ASSERT(m_initialized_type == InitializedType::None);
    if (!m_state->shared_context.has_multiple_extruders || m_state->tool_overrides.empty()) {
        m_initialized_type = InitializedType::PrintOnly;

        m_config_row_item =
            m_header
                ->emplace<ConfigRowItem>(0, m_index, *m_state->print_item, m_cbi_setter, 0);
        m_config_row_item->set_flex_grow(1.f);

    } else {
        m_initialized_type = InitializedType::PrintTool;

        m_main_button = m_header->emplace<PrintToolRowButton>(0);
        m_main_button->set_flex_grow(1.f);
        m_main_button->callbacks().checked_changed = [this](bool checked)
        { m_content->set_visible(checked); };

        m_content = emplace_back<Rectangle>();
        m_content->set_flags(ImDrawFlags_RoundCornersBottom);
        m_content->set_visible(false);
        m_content->set_orientation(Orientation::Vertical);
        m_content->set_gap(5);
        m_content->set_padding(5);
        m_content->set_fill(IM_COL32_BLACK_TRANS);
        m_content->set_border_color(
            m_theme->color_imgui(Platform::Color::Button, Platform::ColorGroup::Active)
        );
        m_content->set_border_width(1);

        m_explanation_container = m_content->emplace_back<Item>();
        m_explanation_container->set_orientation(Orientation::Vertical);
        m_explanation_container->set_gap(5);
        m_explanation_container->set_visible(false);

        m_explanation_labels_list_view =
            m_explanation_container->emplace_back<ExplanationListView>();
        m_explanation_labels_list_view->set_orientation(Orientation::Horizontal);
        m_explanation_labels_list_view->set_gap(5);
        m_explanation_labels_list_view->set_source_list(m_explanation_list_labels.get());

        m_explanation_list_view = m_explanation_container->emplace_back<ExplanationListView>();
        m_explanation_list_view->set_orientation(Orientation::Horizontal);
        m_explanation_list_view->set_gap(5);
        m_explanation_list_view->set_source_list(m_explanation_list.get());

        m_explanation_label = m_explanation_container->emplace_back<Text>(Biz::_u8L(
            "Tool specific values are different for this configuration item, default value is therefore used."
        ));
        m_explanation_label->set_wrap_mode(Text::WrapMode::Wrap);

        m_config_row_item = m_content->emplace_back<ConfigRowItem>(
            m_index,
            *m_state->print_item,
            m_cbi_setter,
            0,
            Biz::_u8L("Default value")
        );

        m_tool_list_view = m_content->emplace_back<ToolRowListView>(
            ToolRowFactory{m_cbi_setter, m_project_interactor}
        );
        m_tool_list_view->set_orientation(Orientation::Vertical);
        m_tool_list_view->set_gap(5);
        m_tool_list_view->set_source_list(this);
    }
}

void PrintToolRowItem::update_explanation()
{
    bool label_visible  = false;
    bool labels_visible = false;
    bool values_visible = false;

    const ImColor text_color = m_theme->color_imgui(Platform::Color::Text);
    const Domain::CompatibilityRule compatibility_rule = m_state->print_item->compatibility_rule();
    if (m_state->print_item->def().require_compatibility_rule && m_state->value.second) {
        if (compatibility_rule == Domain::CompatibilityRule::IgnoreOverrides) {
            label_visible = true;
        } else {
            // construct new explanation list
            std::vector<ExplanationPart> list_labels, list_values;
            list_labels.reserve(16);
            list_values.reserve(16);

            list_labels.emplace_back(
                ExplanationPart{Biz::_u8(m_state->print_item->def().label), text_color}
            );
            list_values.emplace_back(
                ExplanationPart{
                    ConfigItemUtils::config_item_to_string(
                        *m_state->print_item,
                        m_state->value.first
                    ),
                    text_color
                }
            );

            const ExplanationPart equals_part{"=", text_color};
            list_labels.push_back(equals_part);
            list_values.push_back(equals_part);

            ExplanationPart function_part;
            switch (compatibility_rule) {
            case Domain::CompatibilityRule::Average:
                function_part = ExplanationPart{"Average(", text_color};
                break;
            case Domain::CompatibilityRule::Max:
                function_part = ExplanationPart{"Max(", text_color};
                break;
            case Domain::CompatibilityRule::Min:
                function_part = ExplanationPart{"Min(", text_color};
                break;
            default:
                PANIC("Unsuported compability rule type");
                break;
            }

            list_labels.push_back(function_part);
            list_values.push_back(function_part);

            const std::vector<Domain::ColorRGB> colors =
                m_project_interactor.project_settings_interactor().get_colors(
                    m_project_interactor.selected_config_container_id()
                );

            bool inserted = false;
            for (size_t index = 0; index < m_state->tool_overrides.size(); ++index) {
                const Biz::PrintToolItem::ToolOverride& tool_override =
                    m_state->tool_overrides.at(index);
                if (!m_state->shared_context.extruder_candidates.empty()
                    && !m_state->shared_context.extruder_candidates.contains(index))
                {
                    continue;
                }

                if (inserted) {
                    static const ExplanationPart comma_part = ExplanationPart{",", text_color};
                    list_labels.emplace_back(comma_part);
                    list_values.emplace_back(comma_part);
                }

                ImColor color;
                if (colors.size() > index) {
                    const Domain::ColorRGB& color_domain = colors[index];
                    color = {color_domain.r(), color_domain.g(), color_domain.b()};
                } else {
                    m_project_interactor.project_settings_interactor().palette_color(index);
                }

                list_labels.emplace_back(
                    ExplanationPart{Biz::_u8L("Tool") + " " + std::to_string(index + 1), color}
                );
                list_values.emplace_back(
                    ExplanationPart{
                        ConfigItemUtils::config_item_to_string(
                            tool_override.second ? *tool_override.first : *m_state->print_item
                        ),
                        color
                    }
                );

                inserted = true;
            }

            list_labels.emplace_back(ExplanationPart{")", text_color});
            list_values.emplace_back(ExplanationPart{")", text_color});

            auto reset_if_different =
                [](std::vector<ExplanationPart>&& new_list,
                   Biz::UnsharedPointer<Biz::ObservableList<ExplanationPart>>& list) -> void
            {
                bool replace = false;
                if (list->size() != new_list.size()) {
                    replace = true;
                } else {
                    for (size_t index = 0; index < new_list.size(); ++index) {
                        if (list->at(index).first != new_list.at(index).first) {
                            replace = true;
                            break;
                        }
                    }
                }

                if (replace) {
                    list->reset(std::move(new_list));
                }
            };

            reset_if_different(std::move(list_labels), m_explanation_list_labels);
            reset_if_different(std::move(list_values), m_explanation_list);

            labels_visible = true;
            values_visible = true;
        }
    }

    m_explanation_label->set_visible(label_visible);
    m_explanation_list_view->set_visible(values_visible);
    m_explanation_labels_list_view->set_visible(labels_visible);
    m_explanation_container->set_visible(
        m_explanation_label->is_self_visible()
        || m_explanation_labels_list_view->is_self_visible()
        || m_explanation_list_view->is_self_visible()
    );
}

} // namespace Slic3r::App
