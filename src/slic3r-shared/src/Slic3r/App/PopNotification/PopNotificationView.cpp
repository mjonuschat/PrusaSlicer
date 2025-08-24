#include "Slic3r/App/PopNotification/PopNotificationView.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ProgressBar.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"

#include "Slic3r/Directories.hpp"

#include "Slic3r/Log.hpp"

namespace Slic3r::App::PopNotification {

constexpr int TotalWidth = 400;
constexpr int MinHeight  = 40;
constexpr int MaxHeight  = 200;

PopNotificationView::PopNotificationView(size_t index, const PopNotificationData& data, PopNotificationObservableList& notification_list) :
    Biz::DataObserver<PopNotificationData>(index, data),
    Yoga::Window("PopNotification"),
    m_notification_list(notification_list),
    m_current_level(m_state->level())
{
    layout();
}

void PopNotificationView::reset()
{
    // remove child trees from root
    if (m_left_column) {
        remove(m_left_column);
        m_left_column = nullptr;
    }
    if (m_mid_column) {
        remove(m_mid_column);
        m_mid_column = nullptr;
    }
    if (m_right_column) {
        remove(m_right_column);
        m_right_column = nullptr;
    }

    // set rest of the pointers to nullptr (those are already removed)
    m_text   = nullptr;
    m_header = nullptr;
    m_buttons.clear();
    m_button_line  = nullptr;
    m_progress_bar = nullptr;
    m_left_icon    = nullptr;
}

void PopNotificationView::layout()
{
    switch (m_state->layout()) {
    case PopNotificationLayout::Text:
        layout_text();
        break;
    case PopNotificationLayout::HeaderText:
        layout_header_text();
        break;
    case PopNotificationLayout::TextButtons:
        layout_text_buttons();
        break;
    case PopNotificationLayout::HeaderTextButtons:
        layout_header_text_buttons();
        break;
    case PopNotificationLayout::TextProgress:
        layout_text_progress();
        break;
    default:
        ASSERT(false, "Mising layout call");
        break;
    }
    m_current_layout = m_state->layout();
}

void PopNotificationView::on_data_update()
{
    ASSERT(m_state);
    // rebuild whole notification if layout differs
    if (m_current_layout != m_state->layout()) {
        reset();
        layout();
        return;
    }
    if (m_current_level != m_state->level()) {
        m_current_level = m_state->level();
        reset();
        layout();
        return;
    }
    // otherwise check if children needs update
    switch (m_current_layout) {
    case PopNotificationLayout::Text: {
        const auto* layout_data = std::get_if<PopNotificationLayoutText>(&m_state->layout_variant());
        ASSERT(layout_data);
        update_text(layout_data->text);

    } break;
    case PopNotificationLayout::HeaderText: {
        const auto* layout_data = std::get_if<PopNotificationLayoutHeaderText>(&m_state->layout_variant());
        ASSERT(layout_data);
        update_header(layout_data->header);
        update_text(layout_data->text);
    } break;
    case PopNotificationLayout::TextButtons: {
        const auto* layout_data = std::get_if<PopNotificationLayoutTextButtons>(&m_state->layout_variant());
        ASSERT(layout_data);
        update_text(layout_data->text);
        update_buttons(layout_data->buttons);
    } break;
    case PopNotificationLayout::HeaderTextButtons: {
        const auto* layout_data = std::get_if<PopNotificationLayoutHeaderTextButtons>(&m_state->layout_variant());
        ASSERT(layout_data);
        update_header(layout_data->header);
        update_text(layout_data->text);
        update_buttons(layout_data->buttons);
    } break;
    case PopNotificationLayout::TextProgress: {
        const auto* layout_data = std::get_if<PopNotificationLayoutTextProgress>(&m_state->layout_variant());
        ASSERT(layout_data);
        update_text(layout_data->text);
        update_progress(layout_data->progress);
    } break;
    default:
        ASSERT(false, "Mising update call");
        break;
    }
}

void PopNotificationView::basic_layout(Render::Icon icon_override)
{
    set_margin(5.);
    // set_padding(5.);
    set_max_size({TotalWidth, MaxHeight});
    set_min_size({TotalWidth, MinHeight});

    set_orientation(Yoga::Orientation::Horizontal);
    set_justify_content(YGJustifyFlexStart); // razeni itemu uvnitr
    set_self_align(YGAlignFlexEnd); // razeni sebe v listu, end = napravo

    m_left_column  = emplace_back<Yoga::Item>();
    m_mid_column   = emplace_back<Yoga::Item>();
    m_right_column = emplace_back<Yoga::Item>();

    left_layout(icon_override);

    /*
    m_right_column->set_orientation(Yoga::Orientation::Vertical);
    m_right_column->set_justify_content(YGJustifyCenter);
    m_right_column->set_min_size({30, 50});
    */

    m_right_column->set_orientation(Yoga::Orientation::Horizontal);
    m_right_column->set_justify_content(YGJustifyFlexEnd);
    // m_right_column->set_self_align(YGAlignFlexEnd);
    m_right_column->set_flex_grow(1);
    m_right_column->set_min_size({25, MinHeight});

    m_mid_column->set_self_align(YGAlignStretch);
    Yoga::LayoutButton* close_button = m_right_column->emplace_back<Yoga::LayoutButton>("", Render::Icon::PrintIdle);
    close_button->set_min_size({20, 20});
    close_button->set_max_size({20, 20});
    close_button->callbacks().action = [this]()
    {
        SPDLOG_INFO("Notification close button.");
        m_notification_list.on_notification_close_button(m_state->id());
    };

    int left_width  = m_left_column->min_size().x();
    int right_width = m_right_column->min_size().x();
    // m_mid_column->set_min_size({TotalWidth - (left_width + right_width), MinHeight});
    // m_mid_column->set_max_size({TotalWidth - (left_width + right_width), MaxHeight});

    // m_right_column->set_debug_border(true);
    // m_mid_column->set_debug_border(true);
    // m_left_column->set_debug_border(true);
}

void PopNotificationView::left_layout(Render::Icon icon_override)
{
    m_left_column->set_orientation(Yoga::Orientation::Horizontal);
    m_left_column->set_justify_content(YGJustifyFlexStart);

    Render::Icon icon = Render::Icon::None;
    if (icon_override != Render::Icon::None) {
        icon = icon_override;
    } else if (m_state->level() == PopNotificationLevel::Warning) {
        icon = Render::Icon::ErrorMarker;
    } else if (m_state->level() == PopNotificationLevel::Error) {
        icon = Render::Icon::WarningMarker;
    }
    if (icon == Render::Icon::None) {
        m_left_column->set_min_size({0, MinHeight});
        return;
    }

    m_left_column->set_min_size({25, MinHeight});
    m_left_icon = m_left_column->emplace_back<Yoga::Icon>(icon);
    m_left_icon->set_min_size({20, 20});
}

void PopNotificationView::layout_text()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutText>(&m_state->layout_variant());
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    m_mid_column->set_padding(5.);
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyCenter);

    m_text = m_mid_column->emplace_back<Yoga::Text>(layout_data->text);

    m_text->set_text_color(text_color());
}

void PopNotificationView::layout_header_text()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutHeaderText>(&m_state->layout_variant());
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    m_mid_column->set_padding(5.);
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyFlexStart);
    m_header = m_mid_column->emplace_back<Yoga::Text>(layout_data->header);
    m_header->set_font_type(Render::ImguiFontType::Bold);
    m_text = m_mid_column->emplace_back<Yoga::Text>(layout_data->text);

    m_text->set_text_color(text_color());
    m_header->set_text_color(text_color());
}

void PopNotificationView::layout_text_buttons()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutTextButtons>(&m_state->layout_variant());
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    m_mid_column->set_padding(5.);
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyCenter);

    m_text = m_mid_column->emplace_back<Yoga::Text>(layout_data->text);

    m_button_line = m_mid_column->emplace_back<Yoga::Item>();
    m_button_line->set_orientation(Yoga::Orientation::Horizontal);
    m_button_line->set_justify_content(YGJustifyFlexStart);
    m_button_line->set_padding(5.);

    for (auto& bdata : layout_data->buttons) {
        m_buttons.emplace_back(m_button_line->emplace_back<Yoga::LayoutButton>(bdata.text));
        m_buttons.back()->callbacks().action = [bdata, this]()
        {
            ASSERT(bdata.callback);
            if (bdata.callback()) {
                m_notification_list.on_notification_close_button(m_state->id());
            }
        };
        m_buttons.back()->set_background_color(button_color());
    }

    m_text->set_text_color(text_color());
}

void PopNotificationView::layout_header_text_buttons()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutHeaderTextButtons>(&m_state->layout_variant());
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    m_mid_column->set_padding(5.);
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyCenter);

    m_header = m_mid_column->emplace_back<Yoga::Text>(layout_data->header);
    m_header->set_font_type(Render::ImguiFontType::Bold);

    m_text = m_mid_column->emplace_back<Yoga::Text>(layout_data->text);

    m_button_line = m_mid_column->emplace_back<Yoga::Item>();
    m_button_line->set_orientation(Yoga::Orientation::Horizontal);
    m_button_line->set_justify_content(YGJustifyFlexStart);
    m_button_line->set_padding(5.);

    for (auto& bdata : layout_data->buttons) {
        m_buttons.emplace_back(m_button_line->emplace_back<Yoga::LayoutButton>(bdata.text));
        m_buttons.back()->callbacks().action = [bdata, this]()
        {
            ASSERT(bdata.callback);
            if (bdata.callback()) {
                m_notification_list.on_notification_close_button(m_state->id());
            }
        };
        m_buttons.back()->set_background_color(button_color());
    }

    m_text->set_text_color(text_color());
    m_header->set_text_color(text_color());
}

void PopNotificationView::layout_text_progress()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutTextProgress>(&m_state->layout_variant());
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    m_mid_column->set_padding(5.);
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyFlexStart);

    m_text = m_mid_column->emplace_back<Yoga::Text>(layout_data->text);

    m_progress_percent_text = m_mid_column->emplace_back<Yoga::Text>(std::to_string(layout_data->progress) + "%");

    m_progress_bar = m_mid_column->emplace_back<Yoga::ProgressBar>();
    m_progress_bar->set_show_overlay(true);
    m_progress_bar->set_flex_grow(1.f);
    // m_progress_bar->set_progress_fill(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
    // m_progress_bar->set_overlay_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    m_progress_bar->set_progress(layout_data->progress);
    int w = TotalWidth - (m_left_column->min_size().x() + 25 + 25 + 10);
    m_progress_bar->set_min_size({w, 10});
    m_text->set_text_color(text_color());
}

void PopNotificationView::update_text(const std::string& text)
{
    if (text != m_text->text()) {
        m_text->set_text(text);
    }
}

void PopNotificationView::update_header(const std::string& text)
{
    if (text != m_header->text()) {
        m_header->set_text(text);
    }
}

void PopNotificationView::update_buttons(const std::vector<PopNotificationButtonData>& buttons)
{
    for (size_t i = 0; i < buttons.size(); i++) {
        if (m_buttons.size() < i) {
            m_buttons.emplace_back(m_button_line->emplace_back<Yoga::LayoutButton>(buttons[i].text));
        }
        if (m_buttons[i]->label() != buttons[i].text) {
            m_buttons[i]->set_label(buttons[i].text);
        }

        m_buttons[i]->callbacks().action = [cb = buttons[i].callback, this]()
        {
            ASSERT(cb);
            if (cb()) {
                m_notification_list.on_notification_close_button(m_state->id());
            }
        };
    }
}

void PopNotificationView::update_progress(int progress)
{
    if (m_progress_percent_text) {
        m_progress_percent_text->set_text(std::to_string(progress) + "%");
    }
    if (m_progress_bar) {
        m_progress_bar->set_progress(progress);
    }
}

ImColor PopNotificationView::text_color()
{
    if (m_state->level() == PopNotificationLevel::Warning) {
        return {0.98f, 0.4f, 0.19f};
    } else if (m_state->level() == PopNotificationLevel::Error) {
        return {0.79f, 0.17f, 0.17f};
    }

    return {1.0f, 1.0f, 1.0f};
}

ImColor PopNotificationView::button_color()
{
    if (m_state->level() == PopNotificationLevel::Warning) {
        return {0.98f, 0.4f, 0.19f};
    } else if (m_state->level() == PopNotificationLevel::Error) {
        return {0.79f, 0.17f, 0.17f};
    }

    return {0.0f, 0.0f, 0.0f};
}

} // namespace Slic3r::App::PopNotification
