///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Popup.hpp"

#include "Slic3r/App/Yoga/RootItem.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#include <Slic3r/Log.hpp>

#include <imgui_internal.h>

#include <list>

namespace {

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
        rect.Min.y = std::max(0.f, target_rect.GetCenter().y - attachee_size.y * 0.5f);
        break;
    case Slic3r::App::Yoga::Position::Right:
        rect.Min.x = target_rect.Max.x + offset;
        rect.Min.y = std::max(0.f, target_rect.GetCenter().y - attachee_size.y * 0.5f);
        break;
    case Slic3r::App::Yoga::Position::Top:
        rect.Min.x = std::max(0.f, target_rect.GetCenter().x - attachee_size.x * 0.5f);
        rect.Min.y = target_rect.Min.y - offset - attachee_size.y;
        break;
    case Slic3r::App::Yoga::Position::Bottom:
        rect.Min.x = std::max(0.f, target_rect.GetCenter().x - attachee_size.x * 0.5f);
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
    YGNodeStyleSetDisplay(m_popup_node, YGDisplayNone);
}

Popup::~Popup()
{
    close();

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

void Popup::root_item_about_to_update()
{
    // Before any tree change, attempt to close itself
    close();
}

void Popup::on_about_to_show() {}

void Popup::on_about_to_close() {}

void Popup::prepend(ObjectPtr child)
{
    Object::prepend(std::move(child));
}

void Popup::append(ObjectPtr child)
{
    Object::append(std::move(child));
}

void Popup::insert(ObjectPtr child, size_t index)
{
    Object::insert(std::move(child), index);
}

ObjectPtr Popup::remove(Object* child)
{
    return Object::remove(child);
}

Popup::Callbacks& Popup::callbacks()
{
    return m_callbacks;
}

void Popup::attach_to_item(Item* item, Position prefered_position, float offset)
{
    ASSERT(item);
    m_attached_type      = AttachedType::Item;
    m_attached_to        = item;
    m_preferred_position = prefered_position;
    m_offset             = offset;
    m_content_item->set_position_by_yoga(true);
}

void Popup::attach_to_center()
{
    m_attached_type = AttachedType::Center;
    m_attached_to   = nullptr;
    m_content_item->set_position_by_yoga(true);

    YGNodeStyleSetJustifyContent(m_popup_node, YGJustifyCenter);
    YGNodeStyleSetAlignItems(m_popup_node, YGAlignCenter);
}

bool Popup::opened() const
{
    return m_opened;
}

void Popup::open()
{
    ASSERT(m_content_item);

    ASSERT(root_item(), "Cannot open popup which is not inserted into complete tree");

    if (!m_opened) {
        on_about_to_show();

        RootItem* root = dynamic_cast<RootItem*>(root_item());
        root->open_popup(this);
        m_opened = true;
        YGNodeStyleSetDisplay(m_popup_node, YGDisplayFlex);
        m_content_item->set_visible(true);

        if (m_callbacks.opened) {
            m_callbacks.opened();
        }
    }
}

void Popup::close()
{
    ASSERT(m_content_item);
    if (!m_opened) {
        return;
    }

    RootItem* root = dynamic_cast<RootItem*>(root_item());
    ASSERT(root, "Unclosed popup is not inserted into complete tree");
    if (root) {
        on_about_to_close();

        root->close_popup(this);
        m_opened = false;
        YGNodeStyleSetDisplay(m_popup_node, YGDisplayNone);
        m_content_item->set_visible(false);

        if (m_callbacks.closed) {
            m_callbacks.closed();
        }
    }
}

Window* Popup::content_item() const
{
    return m_content_item;
}

void Popup::render(Vec2f pos, Vec2f size)
{
    if (!m_opened || !m_content_item) {
        return;
    }

    if (!Domain::fuzzy_compare(m_last_size.x(), size.x())
        || !Domain::fuzzy_compare(m_last_size.y(), size.y()))
    {
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

void Popup::resize(const Vec2f& size)
{
    m_last_size = size;
    YGNodeCalculateLayout(m_popup_node, size.x(), size.y(), YGDirectionLTR);

    // Free standing windows have all handling implemented in Window class
    if (m_attached_type == AttachedType::FreeStanding) {
        return;
    }

    ImRect popup_rect;
    if (m_attached_type == AttachedType::Item) {
        const Vec2f attachee_global_pos = m_attached_to->get_global_pos();
        const ImRect size_rect(0, 0, size.x(), size.y());
        const ImVec2 target_pos{attachee_global_pos.x(), attachee_global_pos.y()};
        const ImRect target_rect{
            target_pos,
            target_pos + ImVec2{m_attached_to->width(), m_attached_to->height()}
        };
        const ImVec2 attachee_size(m_content_item->width(), m_content_item->height());

        std::list<Position>
            available_positions{Position::Left, Position::Right, Position::Top, Position::Bottom};
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
        popup_rect.Min.x = size.x() * 0.5f - m_content_item->width() * 0.5f;
        popup_rect.Min.y = size.y() * 0.5f - m_content_item->height() * 0.5f;
        popup_rect.Max = popup_rect.Min + ImVec2(m_content_item->width(), m_content_item->height());
    }

    Imgui::move_window_to_bounds({size.x(), size.y()}, popup_rect);

    if (m_attached_type != AttachedType::FreeStanding) {
        if (!Domain::fuzzy_compare(m_content_item->left(), popup_rect.Min.x)) {
            m_content_item->set_left(popup_rect.Min.x);
        }
        if (!Domain::fuzzy_compare(m_content_item->top(), popup_rect.Min.y)) {
            m_content_item->set_top(popup_rect.Min.y);
        }
    }

    YGNodeCalculateLayout(m_popup_node, size.x(), size.y(), YGDirectionLTR);

    // resolve size change
}

void Popup::set_content_item(WindowPtr content_item)
{
    ASSERT(content_item.get());
    if (m_content_item) {
        YGNodeRemoveChild(m_popup_node, m_content_item->node());
        remove(m_content_item);
    }
    m_content_item = content_item.get();
    append(std::move(content_item));
    YGNodeInsertChild(m_popup_node, m_content_item->node(), 0);
    m_content_item->set_position_type(YGPositionTypeAbsolute);
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

} // namespace Slic3r::App::Yoga
