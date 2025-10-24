///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/AlignmentButtons.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "libslic3r/format.hpp"

using namespace Slic3r::Biz;

namespace Slic3r::App::Yoga {

static LayoutButton* add_button(Item* parent, Render::Icon icon, const std::string& tooltip)
{
    LayoutButton* btn = parent->emplace_back<LayoutButton>("", icon, tooltip);
    btn->set_checkable(true);
    btn->set_min_size(Vec2f(24.f, 24.f));
    return btn;
}

AlignmentButtons::AlignmentButtons()
{
    set_gap(10);

    m_left_align_btn   = add_button(this, Render::Icon::AlignHLeftBtn, _u8L("Left"));
    m_center_align_btn = add_button(this, Render::Icon::AlignHCenterBtn, _u8L("Center"));
    m_right_align_btn  = add_button(this, Render::Icon::AlignHRightBtn, _u8L("Right"));

    m_group_horizontal_align.set_buttons({m_left_align_btn, m_center_align_btn, m_right_align_btn});
    m_group_horizontal_align.callbacks().action = [this](AbstractButton* btn) {
        m_align.horizontal = btn == m_left_align_btn ? Domain::HorizontalAlign::left :
            btn == m_right_align_btn                 ? Domain::HorizontalAlign::right :
                                                       Domain::HorizontalAlign::center;
        if (m_callbacks.align_changed)
            m_callbacks.align_changed(m_align);
        update_revert_button();
    };

    m_top_align_btn    = add_button(this, Render::Icon::AlignVTopBtn, _u8L("Top"));
    m_middle_align_btn = add_button(this, Render::Icon::AlignVCenterBtn, _u8L("Middle"));
    m_bottom_align_btn = add_button(this, Render::Icon::AlignVBottomBtn, _u8L("Bottom"));

    m_group_vertical_align.set_buttons({m_top_align_btn, m_middle_align_btn, m_bottom_align_btn});
    m_group_vertical_align.callbacks().action = [this](AbstractButton* btn) {
        m_align.vertical = btn == m_top_align_btn ? Domain::VerticalAlign::top :
            btn == m_bottom_align_btn             ? Domain::VerticalAlign::bottom :
                                                    Domain::VerticalAlign::center;
        if (m_callbacks.align_changed)
            m_callbacks.align_changed(m_align);
        update_revert_button();
    };
}

AlignmentButtons::~AlignmentButtons() {}

AlignmentButtons::Callbacks& AlignmentButtons::callbacks()
{
    return m_callbacks;
}

const Domain::TextAlign& AlignmentButtons::align() const
{
    return m_align;
}

void AlignmentButtons::set_align(const Domain::TextAlign& align)
{
    m_align = align;
    update_revert_button();

    switch (m_align.horizontal) {
    case Domain::HorizontalAlign::left:
        m_left_align_btn->set_checked(true);
        break;
    case Domain::HorizontalAlign::center:
        m_center_align_btn->set_checked(true);
        break;
    case Domain::HorizontalAlign::right:
        m_right_align_btn->set_checked(true);
        break;
    }

    switch (m_align.vertical) {
    case Domain::VerticalAlign::top:
        m_top_align_btn->set_checked(true);
        break;
    case Domain::VerticalAlign::center:
        m_middle_align_btn->set_checked(true);
        break;
    case Domain::VerticalAlign::bottom:
        m_bottom_align_btn->set_checked(true);
        break;
    }
}

void AlignmentButtons::set_default(const Domain::TextAlign& default_align)
{
    m_default_align = default_align;
    update_revert_button();
}

bool AlignmentButtons::is_changed_value() const
{
    return m_align.horizontal != m_default_align.horizontal
        || m_align.vertical != m_default_align.vertical;
}

void AlignmentButtons::reset()
{
    set_align(m_default_align);
}

} // namespace Slic3r::App::Yoga
