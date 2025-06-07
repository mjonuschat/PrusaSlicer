#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Slider.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"

#include "imgui/imgui_internal.h"
#include "Slic3r/Math.hpp"

namespace Slic3r::App::Yoga {

Slider::Callbacks& Slider::callbacks() { return m_callbacks; }

Slider::Slider(float begin, float end, float step) : Oval()
,m_begin_value(begin)
,m_end_value(end)
,m_value(m_begin_value)
,m_step(step)
{
    set_fill(IM_COL32_BLACK_TRANS);
    set_border_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    set_border_width(1.f);

    m_area = emplace_back<Oval>();
    m_area->set_fill(GImGui->Style.Colors[ImGuiCol_TextDisabled]);
    m_area->set_justify_content(YGJustifyFlexEnd);

    m_thumb = m_area->emplace_back<Circle>();
    m_thumb->set_padding(4.f);
    m_thumb->set_min_size({ 14, 14 });
    Circle* knob = m_thumb->emplace_back<Circle>();
    knob->set_fill(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
}

void Slider::process_events(Vec2f pos, Vec2f size)
{
    ImRect ctrl_rc(to_im(pos), to_im(pos+size));
    set_hovered(ImGui::IsMouseHoveringRect(ctrl_rc.Min, ctrl_rc.Max, false));

    // Process interacting with the slider
    float step = m_step;
    if (m_begin_value > m_end_value)
        step *= -1;

    if (m_hovered && ImGui::IsMouseClicked(0)) {
        m_dragging = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    // drag behavior
    if (m_dragging) {
        if (!io.MouseDown[0] || io.MouseReleased[0]) {
            m_dragging = false;
        }
        else {
            const float proc_pos = pos.x() + 0.5f * m_thumb->width();
            const float proc_width = std::max(0.f, width() - m_thumb->width());

            float mouse_abs_pos = io.MousePos[ImGuiAxis_X];
            float mouse_pos_ratio = proc_width == 0.0f ? 0.f : ImClamp((mouse_abs_pos - proc_pos) / proc_width, 0.0f, 1.0f);

            float values_cnt = (m_end_value - m_begin_value) / step;
            float value = m_begin_value + step * std::round(values_cnt * mouse_pos_ratio);
            set_value(value);
        }
    }
    // wheel behavior
    else if (m_hovered) {
        float mw = sign(io.MouseWheel);
        float accer = io.KeyCtrl || io.KeyShift ? 5.0f : 1.0f;
        float value = clamp(m_value + static_cast<float>(mw * accer * step));

        if (!Domain::fuzzy_compare(m_value, value)) {
            set_style_dirty(); // to ask for redraw
        }
        set_value(value);
    }

    Oval::process_events(pos, size);
}

void Slider::set_hovered(bool hovered)
{
    if (m_hovered != hovered) {
        m_hovered = hovered;

        // updated fill on hovering change
        m_area->set_fill(GImGui->Style.Colors[m_hovered ? ImGuiCol_ButtonHovered : ImGuiCol_TextDisabled]);
        m_thumb->set_fill(m_hovered ? GImGui->Style.Colors[ImGuiCol_ButtonActive] : ImGui::ColorConvertU32ToFloat4(IM_COL32_WHITE));
    }
}

float Slider::clamp(float value)
{
    if (m_begin_value < m_end_value)
        return std::clamp(value, m_begin_value, m_end_value);

    return std::clamp(value, m_end_value, m_begin_value);
}

float Slider::snap_to_nearest(float value)
{
    float step = m_step;
    if (m_begin_value > m_end_value)
        step *= -1;

    int pos = std::round((value - m_begin_value) / step);
    return m_begin_value + pos * step;
}

void Slider::set_value(float value)
{
    if (!Domain::fuzzy_compare(m_value, value)) {
        // update value
        m_value = snap_to_nearest(clamp(value));

        if (!Domain::fuzzy_compare(m_end_value, m_begin_value)) {
            float ratio = fabs(m_value - m_begin_value) / fabs(m_end_value - m_begin_value);
            m_area->set_width(std::lerp(m_thumb->width(), width(), ratio));
        }

        if (m_callbacks.value_changed)
            m_callbacks.value_changed(m_value);
    }
}
float Slider::value() const { return m_value; }

float Slider::begin() const { return m_begin_value; }
void Slider::set_begin_value(float begin) { m_begin_value = begin; }

float Slider::end() const { return m_end_value; }
void Slider::set_end_value(float end) { m_end_value = end; }

float Slider::step() const { return m_step; }
void Slider::set_step(float step) { m_step = std::max(step, 0.f); }

} // namespace Slic3r::App::Yoga