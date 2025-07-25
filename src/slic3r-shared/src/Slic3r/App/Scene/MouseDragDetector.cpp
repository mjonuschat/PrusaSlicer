#include "Slic3r/App/Scene/MouseDragDetector.hpp"
#include "Slic3r/Log.hpp"

namespace {
bool is_over_span(const std::chrono::time_point<std::chrono::steady_clock>& start_time, int time_span)
{
    const std::chrono::duration<double> elapsed_seconds{std::chrono::steady_clock::now() - start_time};
    return static_cast<int>(elapsed_seconds.count() * 1'000'000) > time_span;
}

using Slic3r::App::Platform::MouseEvent;

bool is_over_offset(const MouseEvent& me1, const MouseEvent& me2, int offset)
{
    int max_distance = std::max(abs(me1.x() - me2.x()), abs(me1.y() - me2.y()));

    // TODO: To convert on micrometers need DPI of the monitor
    return static_cast<int>(max_distance * (1'000 * 96 / 2.54)) > offset;
}

using Slic3r::App::Scene::DragState;

void interupt(DragState& state)
{
    switch (state) {
    case DragState::no_drag:
        return;
    case DragState::start:
        [[fallthrough]];
    case DragState::dragging:
        state = DragState::interupted;
        return;
    case DragState::start_discard:
        [[fallthrough]];
    case DragState::interupted:
        [[fallthrough]];
    case DragState::finish:
        state = DragState::no_drag;
        return;
    case DragState::start_we_will_see:
        state = DragState::start_discard;
    }
}

void log_weird_state(const std::string& message)
{
    SPDLOG_INFO("Drag detector weird state {}", message);
}
} // namespace

namespace Slic3r::App::Scene {

MouseDragDetector::MouseDragDetector(int min_time_span, int min_offset) :
    m_min_time_span(min_time_span),
    m_min_offset(min_offset)
{}

DragState MouseDragDetector::get_state() const
{
    return m_state;
}

bool MouseDragDetector::is_dragging() const
{
    return m_state == DragState::dragging
        || m_state == DragState::start
        || m_state == DragState::finish;
}

const std::optional<DragStart> MouseDragDetector::get_start() const
{
    return m_start;
}

void MouseDragDetector::mouse_event(const GizmoEventContext& ctx)
{
    const Platform::MouseEvent& me = ctx.mouse_event();
    switch (me.type()) {
    case Platform::MouseEvent::Type::Move:
        switch (m_state) {
        case DragState::no_drag:
            [[fallthrough]];
        case DragState::dragging:
            return;
        case DragState::start_we_will_see:
            if (!m_start.has_value()) { // For sure
                m_state = DragState::no_drag;
            } else if (is_over_span(m_start_time, m_min_time_span)
                       || is_over_offset(m_start->mouse_event, me, m_min_offset))
            {
                m_state = DragState::start;
            }
            return;
        case DragState::start:
            m_state = DragState::dragging;
            return;
        case DragState::finish:
            [[fallthrough]];
        case DragState::start_discard:
            [[fallthrough]];
        case DragState::interupted:
            [[fallthrough]];
        default:
            m_state = DragState::no_drag;
            return;
        }
    case Platform::MouseEvent::Type::ButtonDown:
        switch (me.button()) {
        case Platform::MouseButton::Left:
            if (!me.is_imgui_captured() && can_start_drag()) {
                m_state      = DragState::start_we_will_see;
                m_start_time = std::chrono::steady_clock::now();
                m_start      = DragStart(ctx);
            }
            return;
        case Platform::MouseButton::Right:
            if (m_right_down) {
                log_weird_state("Second Right Button down in row.");
            }
            m_right_down = true;
            interupt(m_state);
            return;
        case Platform::MouseButton::Middle:
            if (m_middle_down) {
                log_weird_state("Second Middle Button down in row.");
            }
            m_middle_down = true;
            interupt(m_state);
            return;
        case Platform::MouseButton::NoButton:
            log_weird_state("Mouse Button down without button.");
            interupt(m_state);
            return;
        default:
            log_weird_state("Unknown mouse button down.");
            interupt(m_state);
            return;
        }
    case Platform::MouseEvent::Type::ButtonUp:
        switch (me.button()) {
        case Platform::MouseButton::Left:
            switch (m_state) {
            case DragState::start:
                [[fallthrough]];
            case DragState::dragging:
                m_state = DragState::finish;
                return;
            case DragState::start_we_will_see:
                m_state = DragState::no_drag;
                return;
            case DragState::start_discard:
                [[fallthrough]];
            case DragState::interupted:
                [[fallthrough]];
            case DragState::finish:
                m_state = DragState::no_drag;
                [[fallthrough]];
            case DragState::no_drag:
                log_weird_state("Button up before down.");
            }
            return;
        case Platform::MouseButton::Right:
            if (!m_right_down) {
                log_weird_state("Right mouse button up before down.");
            }
            m_right_down = false;
            interupt(m_state);
            return;
        case Platform::MouseButton::Middle:
            if (!m_middle_down) {
                log_weird_state("Middle mouse button up before down.");
            }
            m_middle_down = false;
            interupt(m_state);
            return;
        case Platform::MouseButton::NoButton:
            log_weird_state("Mouse Button up without button.");
            interupt(m_state);
            return;
        default:
            log_weird_state("Unknown mouse button down.");
            interupt(m_state);
            return;
        }
    case Platform::MouseEvent::Type::Enter:
        // clear internal state
        m_state = DragState::no_drag;
        // TODO: search for current mouse state and fix it
        // NOTE: When true but in reality false it will be blocking next drag
        m_right_down  = false;
        m_middle_down = false;
        return;
    case Platform::MouseEvent::Type::Wheel:
        [[fallthrough]];
    case Platform::MouseEvent::Type::Leave:
        interupt(m_state);
        return;
    default:
        log_weird_state("Unknown Platform::MouseEvent::Type. " + std::to_string((int) me.type()));
    }
}

void MouseDragDetector::cancel_drag_event()
{
    interupt(m_state);
}

bool MouseDragDetector::can_start_drag()
{
    switch (m_state) {
    case DragState::start_discard:
        [[fallthrough]];
    case DragState::interupted:
        [[fallthrough]];
    case DragState::finish:
        [[fallthrough]];
    case DragState::no_drag:
        m_state = DragState::no_drag;
        return !m_right_down && !m_middle_down;
    case DragState::start:
        [[fallthrough]];
    case DragState::dragging:
        m_state = DragState::interupted;
        log_weird_state("Try to start drag during dragging. Second left mouse button in row");
        return false;
    case DragState::start_we_will_see:
        m_state = DragState::no_drag;
        return false;
    default:
        log_weird_state("Undefined DragState in can start funtion.");
        return false;
    }
}

std::string to_string(DragState state)
{
    switch (state) {
    case DragState::no_drag:
        return "no_drag";
    case DragState::start:
        return "start";
    case DragState::dragging:
        return "dragging";
    case DragState::start_discard:
        return "start_discard";
    case DragState::interupted:
        return "interupted";
    case DragState::finish:
        return "finished";
    case DragState::start_we_will_see:
        return "start_we_will_see";
    default:
        return "undefined";
    }
}

} // namespace Slic3r::App::Scene
