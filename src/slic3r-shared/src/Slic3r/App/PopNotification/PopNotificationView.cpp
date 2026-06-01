#include "Slic3r/App/PopNotification/PopNotificationView.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ProgressBar.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"

namespace Slic3r::App::PopNotification {

constexpr int TotalWidth = 400;
constexpr int MinHeight  = 40;
constexpr int MaxHeight  = 160;

PopNotificationView::PopNotificationView(
    size_t index,
    const PopNotificationData& data,
    PopNotificationObservableList& notification_list
) :
    Yoga::Window("PopNotification"),
    Biz::DataObserver<PopNotificationData>(index, data),
    m_notification_list(notification_list),
    m_current_level(m_state->level)
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
    m_close_button = nullptr;
}

void PopNotificationView::layout()
{
    std::visit(
        Domain::overloaded{
            [this](const PopNotificationLayoutText&) { layout_type_text(); },
            [this](const PopNotificationLayoutHeaderText&) { layout_type_header_text(); },
            [this](const PopNotificationLayoutTextButtons&) { layout_type_text_buttons(); },
            [this](const PopNotificationLayoutHeaderTextButtons&)
            { layout_type_header_text_buttons(); },
            [this](const PopNotificationLayoutTextProgress&) { layout_type_text_progress(); },
            [this](const PopNotificationLayoutHeaderTextProgress&)
            { layout_type_header_text_progress(); }
        },
        m_state->layout
    );

    m_current_layout = m_state->layout;
}

void PopNotificationView::on_data_update()
{
    ASSERT(m_state);

    if (m_current_layout.index() != m_state->layout.index()) {
        reset();
        layout();
        return;
    }

    if (m_current_level != m_state->level) {
        m_current_level = m_state->level;
        reset();
        layout();
        return;
    }

    std::visit(
        Domain::overloaded{
            [this](const PopNotificationLayoutText& d) { update_text(d.text); },
            [this](const PopNotificationLayoutHeaderText& d)
            {
                update_header(d.header);
                update_text(d.text);
            },
            [this](const PopNotificationLayoutTextButtons& d)
            {
                update_text(d.text);
                update_buttons(d.buttons);
            },
            [this](const PopNotificationLayoutHeaderTextButtons& d)
            {
                update_header(d.header);
                update_text(d.text);
                update_buttons(d.buttons);
            },
            [this](const PopNotificationLayoutTextProgress& d)
            {
                update_text(d.text);
                update_progress(d.progress);
            },
            [this](const PopNotificationLayoutHeaderTextProgress& d)
            {
                update_header(d.header);
                update_text(d.text);
                update_progress(d.progress);
            }
        },
        m_state->layout
    );
}

void PopNotificationView::basic_layout(Render::Icon icon_override)
{
    set_margin(5.); // space between notifications

    set_max_width(TotalWidth);
    set_max_height(MaxHeight);
    set_flex_shrink(0.f);
    set_orientation(Yoga::Orientation::Horizontal);
    set_justify_content(YGJustifyFlexStart); // razeni itemu uvnitr

    m_left_column  = emplace_back<Yoga::Item>();
    m_mid_column   = emplace_back<Yoga::Item>();
    m_right_column = emplace_back<Yoga::Item>();

    basic_left_layout(icon_override);
    basic_right_layout();
    m_mid_column->set_flex_grow(1.f);
    m_mid_column->set_self_align(YGAlignStretch);

    m_update_right_on_resize = true;

    // m_right_column->set_debug_border(true);
    // m_mid_column->set_debug_border(true);
    // m_left_column->set_debug_border(true);
}

void PopNotificationView::basic_left_layout(Render::Icon icon_override)
{
    m_left_column->set_orientation(Yoga::Orientation::Horizontal);
    m_left_column->set_justify_content(YGJustifyFlexStart);

    Render::Icon icon = Render::Icon::None;
    if (icon_override != Render::Icon::None) {
        icon = icon_override;
    } else if (m_state->level == PopNotificationLevel::Warning) {
        icon = Render::Icon::ErrorMarker;
    } else if (m_state->level == PopNotificationLevel::Error) {
        icon = Render::Icon::WarningMarker;
    }
    if (icon == Render::Icon::None) {
        m_left_column->set_min_height(MinHeight);
        return;
    }

    m_left_column->set_min_width(25);
    m_left_column->set_min_height(MinHeight);
    m_left_icon = m_left_column->emplace_back<Yoga::Icon>(icon);
    m_left_icon->set_min_width(20);
    m_left_icon->set_min_height(20);
    m_left_icon->set_margin({0.f, 10.f, 10.f, 0.f});
}

void PopNotificationView::basic_right_layout()
{
    if (m_state->level != PopNotificationLevel::ProgressNoClose) {
        m_close_button = m_right_column->emplace_back<Yoga::LayoutButton>(
            std::string{},
            Render::Icon::NotificationCloseGray
        );
        m_close_button->set_min_width(20);
        m_close_button->set_min_height(20);
        m_close_button->set_max_width(20);
        m_close_button->set_max_height(20);
        m_close_button->callbacks().action = [this]()
        {
            // This code is called from render function -> removing notification now might trigger destroying object that is rendering right now.
            Biz::Platform::PlatformServices::instance()
                .main_thread_dispatcher()
                .dispatch_on_main_thread(
                    [this]() { m_notification_list.on_notification_close_button(m_state); }
                );
        };
        m_close_button->set_margin({10.f, 0.f, 0.f, 0.f});
    }
}

void PopNotificationView::basic_mid_layout()
{
    m_mid_column->set_orientation(Yoga::Orientation::Vertical);
    m_mid_column->set_justify_content(YGJustifyCenter);
}

void PopNotificationView::basic_mid_header_layout(const std::string& header)
{
    m_header = m_mid_column->emplace_back<Yoga::Text>(header);
    m_header->set_font_type(Render::ImguiFontType::Bold);
    m_header->set_text_color(text_color());
    m_header->set_margin({0.f, 5.f, 0.f, 5.f});
    m_header->set_flex_shrink(0.f);
    m_header->set_width_percent(100.f);  
    m_header->set_wrap_mode(Yoga::Text::WrapMode::WrapElide);
}

void PopNotificationView::basic_mid_text_layout(const std::string& text)
{
    auto* text_scroll = m_mid_column->emplace_back<Yoga::ScrollArea>("TextScroll");
    text_scroll->set_flex_grow(1.f);
    text_scroll->set_flex_shrink(1.f);
    text_scroll->set_margin({0.f, 5.f, 0.f, 10.f});
    text_scroll->set_width_percent(100.f); 

    m_text = text_scroll->emplace_back<Yoga::Text>(text);
    m_text->set_wrap_mode(Yoga::Text::WrapMode::Wrap);
    m_text->set_text_color(text_color());
    m_text->set_flex_shrink(0.f);
    m_text->set_width_percent(100.f);
}

void PopNotificationView::basic_mid_buttons_layout(
    const std::vector<PopNotificationButtonData>& buttons
)
{
    m_button_line = m_mid_column->emplace_back<Yoga::Item>();
    m_button_line->set_orientation(Yoga::Orientation::Horizontal);
    m_button_line->set_justify_content(YGJustifyFlexStart);
    m_button_line->set_margin({0.f, 0.f, 0.f, 5.f});
    m_button_line->set_flex_shrink(0.f);
    for (const auto& bdata : buttons) {
        Yoga::LayoutButton* button =
            m_buttons.emplace_back(m_button_line->emplace_back<Yoga::LayoutButton>(bdata.text));
        button->callbacks().action = [bdata, this]()
        {
            ASSERT(bdata.callback);
            Biz::Platform::PlatformServices::instance()
                .main_thread_dispatcher()
                .dispatch_on_main_thread(
                    [cb = bdata.callback, this, state_to_close = this->m_state]()
                    {
                        if (cb()) {
                            m_notification_list.on_notification_close_button(state_to_close);
                        }
                    }
                );
        };
        button->set_background_color(button_color());
        button->set_margin({0.f, 0.f, 5.f, 0.f});
        if (button->label_object()) {
            button->label_object()->set_margin({5.f, 2.f, 5.f, 2.f});
        }
    }
}

void PopNotificationView::basic_mid_progress_layout(int progress)
{
    m_progress_bar = m_mid_column->emplace_back<Yoga::ProgressBar>();
    m_progress_bar->set_show_overlay(true);
    m_progress_bar->set_flex_grow(1.f);
    m_progress_bar->set_progress(progress);
    m_progress_bar->set_min_width(m_mid_column->min_width());
    m_progress_bar->set_min_height(3);
    m_progress_bar->set_margin({0.f, 5.f, 0.f, 0.f});
    m_progress_bar->set_flex_shrink(0.f);

    m_progress_percent_text =
        m_mid_column->emplace_back<Yoga::Text>(std::to_string(progress) + "%");
    m_progress_percent_text->set_margin({0.f, 0.f, 0.f, 0.f});
    m_progress_percent_text->set_flex_shrink(0.f);
}

void PopNotificationView::layout_type_text()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutText>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_text_layout(layout_data->text);
}

void PopNotificationView::layout_type_header_text()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutHeaderText>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_header_layout(layout_data->header);
    basic_mid_text_layout(layout_data->text);
}

void PopNotificationView::layout_type_text_buttons()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutTextButtons>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_text_layout(layout_data->text);
    basic_mid_buttons_layout(layout_data->buttons);
}

void PopNotificationView::layout_type_header_text_buttons()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutHeaderTextButtons>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_header_layout(layout_data->header);
    basic_mid_text_layout(layout_data->text);
    basic_mid_buttons_layout(layout_data->buttons);
}

void PopNotificationView::layout_type_text_progress()
{
    const auto* layout_data = std::get_if<PopNotificationLayoutTextProgress>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_text_layout(layout_data->text);
    basic_mid_progress_layout(layout_data->progress);
}

void PopNotificationView::layout_type_header_text_progress()
{
    const auto* layout_data =
        std::get_if<PopNotificationLayoutHeaderTextProgress>(&m_state->layout);
    ASSERT(layout_data);
    basic_layout(Render::Icon::None);
    basic_mid_layout();
    basic_mid_header_layout(layout_data->header);
    basic_mid_text_layout(layout_data->text);
    basic_mid_progress_layout(layout_data->progress);
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
        if (m_buttons.size() <= i) {
            m_buttons.emplace_back(
                m_button_line->emplace_back<Yoga::LayoutButton>(buttons[i].text)
            );
        }
        if (m_buttons[i]->label() != buttons[i].text) {
            m_buttons[i]->set_label(buttons[i].text);
        }

        m_buttons[i]->callbacks().action = [cb = buttons[i].callback, this]()
        {
            ASSERT(cb);
            // This code is called from render function -> removing notification now might trigger destroying object that is rendering right now.
            Biz::Platform::PlatformServices::instance()
                .main_thread_dispatcher()
                .dispatch_on_main_thread(
                    [cb, this, state_to_close = this->m_state]()
                    {
                        if (cb()) {
                            m_notification_list.on_notification_close_button(state_to_close);
                        }
                    }
                );
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

ImColor PopNotificationView::text_color() const
{
    if (m_state->level == PopNotificationLevel::Warning) {
        return m_theme->color_imgui(Platform::Color::Warning);
    } else if (m_state->level == PopNotificationLevel::Error) {
        return m_theme->color_imgui(Platform::Color::Error);
    }

    return m_theme->color_imgui(Platform::Color::Text);
}

Platform::Color PopNotificationView::button_color() const
{
    switch (m_state->level) {
    case PopNotificationLevel::Warning:
        return Platform::Color::Warning;
    case PopNotificationLevel::Error:
        return Platform::Color::Error;
    default:
        return Platform::Color::Button;
    }
}

void PopNotificationView::on_resized()
{
    if (m_update_right_on_resize && height() > 0) {
        m_update_right_on_resize = false;
        update_right_column();
    }
}

void PopNotificationView::update_right_column()
{
    if (height() > 0 && height() <= 70) {
        m_right_column->set_orientation(Yoga::Orientation::Vertical);
        m_right_column->set_justify_content(YGJustifyCenter);
    } else {
        m_right_column->set_orientation(Yoga::Orientation::Horizontal);
        m_right_column->set_justify_content(YGJustifyFlexEnd);
    }
}

} // namespace Slic3r::App::PopNotification
