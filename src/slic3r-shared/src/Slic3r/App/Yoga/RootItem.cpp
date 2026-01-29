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
    set_object_name("RootItem");
}

RootItem::~RootItem()
{
    // Manually mark all popups as closed (RootItem is deleted first)
    for (Popup* popup : std::as_const(m_popups)) {
        popup->close();
    }

    while (!m_popups_to_be_added.empty()) {
        m_popups_to_be_added.front()->close();
    }
}

template <class F>
void RootItem::for_each_popup_reconcile(F&& fn)
{
    for (Popup* deleted_popup : std::as_const(m_popups_to_be_deleted)) {
        Popups::const_iterator deleted_it =
            std::find(m_popups.cbegin(), m_popups.cend(), deleted_popup);
        ASSERT(deleted_it != m_popups.cend());
        m_popups.erase(deleted_it);
    }
    m_popups_to_be_deleted.clear();

    for (Popups::iterator it = m_popups.begin(); it != m_popups.end();) {
        Popup* current = *it;
        fn(*current);

        if (!m_popups_to_be_deleted.empty()) {
            bool erased_current = false;

            for (Popup* deleted : std::as_const(m_popups_to_be_deleted)) {
                Popups::iterator deleted_it = std::find(m_popups.begin(), m_popups.end(), deleted);

                ASSERT(deleted_it != m_popups.cend());
                if (deleted_it == it) {
                    it             = m_popups.erase(it);
                    erased_current = true;
                } else {
                    m_popups.erase(deleted_it);
                }
            }
            m_popups_to_be_deleted.clear();

            if (erased_current) {
                continue;
            }
        }

        ++it;
    }
}

void RootItem::render(Vec2f pos, Vec2f size)
{
    ASSERT(!parent());

    if (size.isZero()) {
        return;
    }

    if (!m_popups_to_be_added.empty()) {
        std::copy(
            m_popups_to_be_added.cbegin(),
            m_popups_to_be_added.cend(),
            std::back_inserter(m_popups)
        );
        m_popups_to_be_added.clear();
    }

    m_size = size;

    style_node();
    m_style_dirty = false;
    resize(size);
    if (m_style_dirty) {
        style_node();
        resize(size);
    }

    render_item_begin(pos, size);

    render_item_end(pos, size);

    for_each_popup_reconcile([&](Popup& popup) { popup.render({}, size); });

    render_debug_overlay();

    if (m_loop_events.process_events() || m_style_dirty) {
        // There were a number of processed events,
        // therefore we need to schedule another render pass
        // OR
        // Some item called set_style_dirty in render()
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
        if (!m_debug_item->object_name().empty()) {
            text += m_debug_item->object_name() + " ";
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

    for_each_popup_reconcile([&](Popup& popup) { popup.resize(size); });

    check_resized();

    for_each_popup_reconcile([&](Popup& popup) { popup.check_resized(); });
}

void RootItem::style_node()
{
    Item::style_node();

    for_each_popup_reconcile([&](Popup& popup) { popup.style_node(); });
}

void RootItem::open_popup(Popup* popup)
{
    ASSERT(popup);
    ASSERT(popup->content_item());

    Popups::const_iterator it =
        std::find(m_popups_to_be_deleted.cbegin(), m_popups_to_be_deleted.cend(), popup);
    if (it != m_popups_to_be_deleted.cend()) {
        m_popups_to_be_deleted.erase(it);
    }

    if (std::find(m_popups.cbegin(), m_popups.cend(), popup) != m_popups.cend()
        || std::find(m_popups_to_be_added.cbegin(), m_popups_to_be_added.cend(), popup)
            != m_popups_to_be_added.cend())
    {
        return;
    }

    m_popups_to_be_added.push_back(popup);
}

void RootItem::close_popup(Popup* popup)
{
    if (std::find(m_popups_to_be_deleted.cbegin(), m_popups_to_be_deleted.cend(), popup)
        != m_popups_to_be_deleted.cend())
    {
        // Popup is already scheduled to be deleted
        return;
    }

    Popups::const_iterator it =
        std::find(m_popups_to_be_added.cbegin(), m_popups_to_be_added.cend(), popup);
    if (it != m_popups_to_be_added.cend()) {
        // Popup was just scheduled to be opened
        m_popups_to_be_added.erase(it);
        return;
    }

    if (std::find(m_popups.cbegin(), m_popups.cend(), popup) != m_popups.cend()) {
        // Popup is scheduled to be deleted
        m_popups_to_be_deleted.push_back(popup);
    }
}

} // namespace Slic3r::App::Yoga
