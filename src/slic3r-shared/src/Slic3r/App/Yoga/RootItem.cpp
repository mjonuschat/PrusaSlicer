///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/RootItem.hpp"

#include "Slic3r/App/Yoga/Popup.hpp"
#include "Slic3r/Biz/Platform/PlatformServices.hpp"

#include <imgui_internal.h>
#include <fmt/format.h>

namespace Slic3r::App::Yoga {

RootItem::RootItem() : m_loop_events(*this)
{
    set_item_name("RootItem");
}

void RootItem::render(Vec2f pos, Vec2f size)
{
    ASSERT(!m_parent);

    if (size.isZero()) {
        return;
    }

    m_size = size;

    style_node();
    m_style_dirty = false;
    resize(size);
    process_events(pos, size);
    if (m_style_dirty) {
        style_node();
        resize(size);
    }

    render_item_begin(pos, size);

    render_item_end(pos, size);

    for (Popup* popup : std::as_const(m_popups)) {
        popup->render(size);
    }

    render_debug_overlay();

    if (m_loop_events.process_events()) {
        // There were a number of processed events,
        // therefore we need to schedule another render pass
        Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
    }
}

void RootItem::set_style_dirty()
{
    m_style_dirty = true;
}

void RootItem::push_event(EventPtr event)
{
    m_loop_events.insert_event(std::move(event));
}

Vec2f RootItem::get_available_size() const
{
    return m_size;
}

void RootItem::render_debug_overlay()
{
#ifdef DEBUG
    // In Debug if m_debug_item is set, draw a rect around the item and show debug info
    if (m_debug_item) {
        const Vec2f global_pos = m_debug_item->get_global_pos();
        const Vec2f size       = Vec2f(m_debug_item->width(), m_debug_item->height());

        ImRect rect(to_im(global_pos), to_im(global_pos + size));
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddRect(rect.Min, rect.Max, IM_COL32(255, 0, 0, 128));

        std::string text;
        if (!m_debug_item->item_name().empty()) {
            text += m_debug_item->item_name() + " ";
        }
        text += fmt::format(
            "{}x{}px at pos [{}]:[{}]",
            size.x(),
            size.y(),
            global_pos.x(),
            global_pos.y()
        );

        ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
        ImVec2 pad       = ImVec2(6.0f, 4.0f); // padding around the text

        // use main viewport pos+size for absolute coords
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 bottom_left            = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y);

        // rectangle covering the text at bottom-left
        ImVec2 rect_min = ImVec2(bottom_left.x, bottom_left.y - text_size.y - pad.y * 2.0f);
        ImVec2 rect_max = ImVec2(bottom_left.x + text_size.x + pad.x * 2.0f, bottom_left.y);

        draw_list->AddRectFilled(rect_min, rect_max, IM_COL32(0, 0, 0, 200));
        draw_list->AddText(
            ImVec2(rect_min.x + pad.x, rect_min.y + pad.y),
            IM_COL32(255, 0, 0, 255),
            text.c_str()
        );
        m_debug_item = nullptr;
    }
#endif
}

void RootItem::resize(Vec2f size)
{
    YGNodeCalculateLayout(m_node, size.x(), size.y(), YGDirectionLTR);

    for (Popup* popup : std::as_const(m_popups)) {
        popup->resize(size);
    }

    check_resized();

    for (Popup* popup : std::as_const(m_popups)) {
        popup->check_resized();
    }
}

void RootItem::style_node()
{
    Item::style_node();

    for (Popup* popup : std::as_const(m_popups)) {
        popup->style_node();
    }
}

void RootItem::process_events(Vec2f pos, Vec2f size)
{
    Item::process_events(pos, size);

    // Number of popups can be changed inside process events
    // and so we are creating an immutable copy
    // TODO: a deffered popup insert/close should be implemented
    const Popups popups = m_popups;
    for (Popup* popup : std::as_const(m_popups)) {
        popup->process_events(pos, size);
    }
}

void RootItem::open_popup(Popup* popup)
{
    ASSERT(popup);
    ASSERT(popup->content_item());

    Popups::const_iterator it = std::find(m_popups.cbegin(), m_popups.cend(), popup);
    if (it != m_popups.cend()) {
        return;
    }

    m_popups.push_back(popup);
}

void RootItem::close_popup(Popup* popup)
{
    Popups::const_iterator it = std::find(m_popups.cbegin(), m_popups.cend(), popup);
    if (it == m_popups.cend()) {
        return;
    }

    m_popups.erase(it);
}

} // namespace Slic3r::App::Yoga
