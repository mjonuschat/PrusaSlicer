///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Popup.hpp"

#include "Slic3r/App/Yoga/RootItem.hpp"

#include <Slic3r/Log.hpp>

#include <imgui_internal.h>

#include <list>

namespace {

void move_window_to_bounds(const Slic3r::Domain::Vec2f& available_size, ImRect& window)
{
    {
        // clamp window size to available_size
        const ImVec2 window_size = window.GetSize();
        if (window_size.x > available_size.x()) {
            window.Max.x -= window_size.x - available_size.x();
        }
        if (window_size.y > available_size.y()) {
            window.Max.y -= window_size.y - available_size.y();
        }
    }

    // Move window to range [0,0]-[available_size.x, available_size.y]

    // left
    if (window.Min.x < 0) {
        window.TranslateX(abs(window.Min.x));
    }
    // right
    if (window.Max.x > available_size.x()) {
        window.TranslateX(available_size.x() - window.Max.x);
    }
    // top
    if (window.Min.y < 0) {
        window.TranslateY(abs(window.Min.y));
    }
    // bottom
    if (window.Max.y > available_size.y()) {
        window.TranslateY(available_size.y() - window.Max.y);
    }
}

[[nodiscard]] std::pair<bool, ImRect> try_to_place_popup(
    const ImRect& size_rect,
    const ImRect& target_rect,
    const ImVec2& attachee_size,
    float offset,
    Slic3r::App::Yoga::Position position
)
{
    ImRect rect;
    switch (position) {
    case Slic3r::App::Yoga::Position::Left:
        rect.Min.x = target_rect.Min.x - offset - attachee_size.x;
        rect.Min.y = target_rect.GetCenter().y - attachee_size.y * 0.5;
        break;
    case Slic3r::App::Yoga::Position::Right:
        rect.Min.x = target_rect.Max.x + offset;
        rect.Min.y = target_rect.GetCenter().y - attachee_size.y * 0.5;
        break;
    case Slic3r::App::Yoga::Position::Top:
        rect.Min.x = target_rect.GetCenter().x - attachee_size.x * 0.5;
        rect.Min.y = target_rect.Min.y - offset - attachee_size.y;
        break;
    case Slic3r::App::Yoga::Position::Bottom:
        rect.Min.x = target_rect.GetCenter().x - attachee_size.x * 0.5;
        rect.Min.y = target_rect.Max.y + offset;
        break;
    }

    rect.Max = rect.Min + attachee_size;

    return {size_rect.Contains(rect), {rect}};
}

} // namespace

namespace Slic3r::App::Yoga {

Popup::Popup()
{
    m_popup_node = YGNodeNew();
}

Popup::~Popup()
{
    if (m_root_item) {
        close();
    }

    YGNodeFree(m_popup_node);
}

Position Popup::preferred_position() const
{
    return m_preferred_position;
}

void Popup::set_preferred_position(Position preferred_position)
{
    m_preferred_position = preferred_position;
}

RootItem* Popup::get_or_find_root_item()
{
    // We were never parented, that means we are not opened and we can be destroyed in silence
    if (!m_parent) {
        ASSERT(!m_opened);
        return nullptr;
    }

    if (!m_root_item) {
        find_root_item();
    }

    return m_root_item;
}

void Popup::set_root_item(RootItem* root_item)
{
    m_root_item = root_item;
}

void Popup::on_about_to_show() {}

Popup::Callbacks& Popup::callbacks()
{
    return m_callbacks;
}

void Popup::attach_to_item(Item* item, Position prefered_position, float offset)
{
    ASSERT(item);
    m_parent             = item;
    m_attached_type      = AttachedType::Item;
    m_attached_to        = item;
    m_preferred_position = prefered_position;
    m_offset             = offset;
    m_content_item->set_position_by_yoga(true);
}

void Popup::attach_to_center(Item* item)
{
    ASSERT(item);
    m_parent        = item;
    m_attached_type = AttachedType::Center;
    m_attached_to   = nullptr;
    m_content_item->set_position_by_yoga(true);

    YGNodeStyleSetJustifyContent(m_popup_node, YGJustifyCenter);
    YGNodeStyleSetAlignItems(m_popup_node, YGAlignCenter);
}

void Popup::detach(Item* item)
{
    ASSERT(item);
    m_parent        = item;
    m_attached_type = AttachedType::FreeStanding;
    m_attached_to   = nullptr;
    m_content_item->set_position_by_yoga(false);
}

bool Popup::opened() const
{
    return m_opened;
}

void Popup::open()
{
    ASSERT(m_content_item.get());

    if (!m_root_item) {
        find_root_item();
    }

    if (m_root_item && !m_opened) {
        on_about_to_show();

        m_root_item->open_popup(this);
        m_opened = true;

        if (m_callbacks.opened) {
            m_callbacks.opened();
        }
    }
}

void Popup::close()
{
    ASSERT(m_content_item.get());
    if (!m_opened) {
        return;
    }

    if (!m_root_item) {
        find_root_item();
    }

    if (m_root_item) {
        m_root_item->close_popup(this);
        m_opened = false;

        if (m_callbacks.closed) {
            m_callbacks.closed();
        }
    }
}

Window* Popup::content_item() const
{
    return m_content_item.get();
}

void Popup::render(const Vec2f& size)
{
    if (!m_resized) {
        style_node();
        resize(size);
    }

    m_content_item->render(
        {m_content_item->x(), m_content_item->y()},
        {m_content_item->width(), m_content_item->height()}
    );
}

void Popup::check_resized()
{
    m_content_item->check_resized();
}

void Popup::style_node()
{
    m_content_item->style_node();
}

void Popup::resize(const Vec2f& size)
{
    m_resized = true;
    YGNodeCalculateLayout(m_popup_node, size.x(), size.y(), YGDirectionLTR);

    // Free standing windows have all handling implemented in Window class
    if (m_attached_type == AttachedType::FreeStanding) {
        return;
    }

    ImRect popup_rect;
    if (m_attached_type == AttachedType::Item) {
        const Vec2f attachee_global_pos = m_parent->get_global_pos();
        const ImRect size_rect(0, 0, size.x(), size.y());
        const ImVec2 target_pos{attachee_global_pos.x(), attachee_global_pos.y()};
        const ImRect target_rect{target_pos, target_pos + ImVec2{m_parent->width(), m_parent->height()}};
        const ImVec2 attachee_size(m_content_item->width(), m_content_item->height());

        std::list<Position> available_positions{
            Position::Left,
            Position::Right,
            Position::Top,
            Position::Bottom
        };
        std::erase(available_positions, m_preferred_position);

        std::pair<bool, ImRect> placed = try_to_place_popup(
            size_rect,
            target_rect,
            attachee_size,
            m_offset,
            m_preferred_position
        );
        ImRect preferred_rect = placed.second;
        // fallback to all other positions
        while (!placed.first && !available_positions.empty()) {
            placed = try_to_place_popup(
                size_rect,
                target_rect,
                attachee_size,
                m_offset,
                available_positions.front()
            );
            available_positions.pop_front();
        }
        popup_rect = placed.first ? placed.second : preferred_rect;
    } else if (m_attached_type == AttachedType::Center) {
        popup_rect.Min.x = size.x() * 0.5 - m_content_item->width() * 0.5;
        popup_rect.Min.y = size.y() * 0.5 - m_content_item->height() * 0.5;
        popup_rect.Max = popup_rect.Min + ImVec2(m_content_item->width(), m_content_item->height());
    }

    move_window_to_bounds(size, popup_rect);

    if (m_attached_type != AttachedType::FreeStanding) {
        m_content_item->set_left(popup_rect.Min.x);
        m_content_item->set_top(popup_rect.Min.y);
    }

    YGNodeCalculateLayout(m_popup_node, size.x(), size.y(), YGDirectionLTR);

    // resolve size change
}

void Popup::process_events(Vec2f pos, Vec2f size)
{
    m_content_item->process_events(
        {m_content_item->x(), m_content_item->y()},
        {m_content_item->width(), m_content_item->height()}
    );
}

void Popup::set_content_item(WindowPtr content_item)
{
    ASSERT(content_item.get());
    YGNodeRemoveAllChildren(m_popup_node);
    m_content_item = std::move(content_item);
    if (m_content_item) {
        YGNodeInsertChild(m_popup_node, m_content_item.get()->node(), 0);
        m_content_item->set_position_type(YGPositionTypeAbsolute);
        m_content_item->set_parent_popup(this);
    }
}

void Popup::find_root_item()
{
    Item* parent = m_parent;
    while (parent) {
        // TODO: OOF, this won't work in the destructor, RootItem needs a more
        // comprehensive handling!
        RootItem* root_item = dynamic_cast<RootItem*>(parent);
        if (root_item) {
            m_root_item = root_item;
            break;
        }
        if (parent->parent_popup()) {
            m_root_item = parent->parent_popup()->get_or_find_root_item();
            break;
        }

        parent = parent->parent();
    }
    if (!m_root_item) {
        SPDLOG_WARN("RootItem was not found");
    }
    // ASSERT(m_root_item, "RootItem was not found");
}

float Popup::offset() const
{
    return m_offset;
}

void Popup::set_offset(float offset)
{
    m_offset = offset;
}

void Popup::open_at(const Vec2f& pos)
{
    m_content_item->request_position(pos);
    open();
}

void Popup::open_at(Item* item, Position prefered_position, float offset) {}

} // namespace Slic3r::App::Yoga
