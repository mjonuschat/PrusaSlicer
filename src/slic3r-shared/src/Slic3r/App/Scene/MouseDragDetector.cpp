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

namespace {
template<class Callback>
void add_callback(std::vector<std::pair<IGizmo*, Callback>>& callbacks, IGizmo* gizmo, Callback&& callback) {
    // sorted insert
    auto pred = [](IGizmo* gizmo, const std::pair<IGizmo*, Callback>& c) { return gizmo < c.first; };
    auto it = std::upper_bound(callbacks.begin(), callbacks.end(), gizmo, pred);
    if (it == callbacks.end()) {
        if (callback != nullptr) // sorted insert
            callbacks.emplace_back(gizmo, std::move(callback));
    } else if (it->first == gizmo) {
        if (callback == nullptr) {
            // free callback
            callbacks.erase(it);
        } else {
            // override callback for gizmo
            it->second = std::move(callback);
        }
    } else {
        // sorted insert
        callbacks.insert(it, std::pair<IGizmo*, Callback>(gizmo, callback));
    }
}
}

void MouseDragDetector::add_on_start(IGizmo* gizmo, IMouseDragCallbacks::OnStart callback) { add_callback(m_on_starts, gizmo, std::move(callback)); }
void MouseDragDetector::add_on_drag(IGizmo* gizmo, IMouseDragCallbacks::OnDrag callback) { add_callback(m_on_drags, gizmo, std::move(callback)); }
void MouseDragDetector::add_on_finish(IGizmo* gizmo, IMouseDragCallbacks::OnFinish callback) { add_callback(m_on_finishes, gizmo, std::move(callback)); }
void MouseDragDetector::add_on_cancel(IGizmo* gizmo, IMouseDragCallbacks::OnCancel callback) { add_callback(m_on_cancels, gizmo, std::move(callback)); }

bool MouseDragDetector::on_start(const std::vector<IGizmo*>& gizmos) {
    GizmoEventContext ctx = m_start->create_ctx();
    auto pred = [](const std::pair<IGizmo*, IMouseDragCallbacks::OnStart>& c, const IGizmo* gizmo) { return gizmo < c.first; };
    for (const IGizmo* gizmo : gizmos) {
        auto it = std::lower_bound(m_on_starts.cbegin(), m_on_starts.cend(), gizmo, pred);
        if (it != m_on_starts.cend() && 
            it->first == gizmo &&
            it->second(ctx)) {
            // gizmo consume drag
            m_dragging_gizmo = gizmo;
            return true; 
        }
    }
    m_dragging_gizmo = nullptr;
    return false;
}

namespace {
bool call_dragging(const GizmoEventContext& ctx, const IGizmo* gizmo, const std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnDrag>>& on_drags) {
    if (gizmo == nullptr)
        return false;

    auto pred = [](const std::pair<IGizmo*, IMouseDragCallbacks::OnDrag>& c, const IGizmo* gizmo) { return gizmo < c.first; };
    auto it = std::lower_bound(on_drags.begin(), on_drags.end(), gizmo, pred);
    if (it != on_drags.end() && it->first == gizmo) {
        it->second(ctx);
        return true;
    }
    return false;
}
void call_finish(const IGizmo* gizmo, const std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnFinish>>& on_finishes) {
    if (gizmo == nullptr)
        return;

    auto pred = [](const std::pair<IGizmo*, IMouseDragCallbacks::OnFinish>& c, const IGizmo* gizmo) { return gizmo < c.first; };
    auto it = std::lower_bound(on_finishes.begin(), on_finishes.end(), gizmo, pred);
    if (it != on_finishes.end() && it->first == gizmo) {
        it->second();
    }
}

}

bool MouseDragDetector::mouse_event(const GizmoEventContext& ctx, const std::vector<IGizmo*>& gizmos)
{
    const Platform::MouseEvent& me = ctx.mouse_event();
    switch (me.type()) {
    case Platform::MouseEvent::Type::Move:
        switch (m_state) {
        case DragState::NoDrag:
            return false;
        case DragState::Dragging:
            return call_dragging(ctx, m_dragging_gizmo, m_on_drags);
        case DragState::StartWeWillSee:
            if (!m_start.has_value()) { // For sure
                log_weird_state("Missing start data");
                m_state = DragState::NoDrag;
            } else if (is_over_span(m_start_time, m_min_time_span)
                       || is_over_offset(m_start->mouse_event, me, m_min_offset))
            {
                m_state = DragState::Dragging;
                return on_start(gizmos);
            }
            return false;
        }
        return false;
    case Platform::MouseEvent::Type::ButtonDown:
        switch (me.button()) {
        case Platform::MouseButton::Left:
            if (!me.is_imgui_captured() && can_start_drag()) {
                m_state      = DragState::StartWeWillSee;
                m_start_time = std::chrono::steady_clock::now();
                m_start      = DragStart(ctx);
            }
            return false;
        case Platform::MouseButton::Right:
            if (m_right_down) {
                log_weird_state("Second Right Button down in row.");
            }
            m_right_down = true;
            cancel_drag_event();
            return false;
        case Platform::MouseButton::Middle:
            if (m_middle_down) {
                log_weird_state("Second Middle Button down in row.");
            }
            m_middle_down = true;
            cancel_drag_event();
            return false;
        case Platform::MouseButton::NoButton:
            log_weird_state("Mouse Button down without button.");
            cancel_drag_event();
            return false;
        default:
            log_weird_state("Unknown mouse button down.");
            cancel_drag_event();
            return false;
        }
    case Platform::MouseEvent::Type::ButtonUp:
        switch (me.button()) {
        case Platform::MouseButton::Left:
            switch (m_state) {
            case DragState::Dragging:
                call_finish(m_dragging_gizmo, m_on_finishes);
                m_state = DragState::NoDrag;
                break;
            case DragState::StartWeWillSee:
                m_state = DragState::NoDrag;
                break;
            case DragState::NoDrag:
                log_weird_state("Button up before down.");
            }
            return false;
        case Platform::MouseButton::Right:
            if (!m_right_down) {
                log_weird_state("Right mouse button up before down.");
            }
            m_right_down = false;
            cancel_drag_event();
            return false;
        case Platform::MouseButton::Middle:
            if (!m_middle_down) {
                log_weird_state("Middle mouse button up before down.");
            }
            m_middle_down = false;
            cancel_drag_event();
            return false;
        case Platform::MouseButton::NoButton:
            log_weird_state("Mouse Button up without button.");
            cancel_drag_event();
            return false;
        default:
            log_weird_state("Unknown mouse button down.");
            cancel_drag_event();
            return false;
        }
    case Platform::MouseEvent::Type::Enter:
        // clear internal state
        m_state = DragState::NoDrag;
        // TODO: search for current mouse state and fix it
        // NOTE: When true but in reality false it will be blocking next drag
        m_right_down  = false;
        m_middle_down = false;
        break;
    case Platform::MouseEvent::Type::Wheel:
        [[fallthrough]];
    case Platform::MouseEvent::Type::Leave:
        cancel_drag_event();
        return false;
    default:
        log_weird_state("Unknown Platform::MouseEvent::Type. " + std::to_string((int) me.type()));
    }
    return false;
}

namespace {
void call_cancel(const IGizmo* gizmo, const std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnCancel>>& on_cancels) {
    if (gizmo == nullptr)
        return;
    
    auto pred = [](const std::pair<IGizmo*, IMouseDragCallbacks::OnCancel>& c, const IGizmo* gizmo) { return gizmo < c.first; };
    auto it = std::lower_bound(on_cancels.begin(), on_cancels.end(), gizmo, pred);
    if (it != on_cancels.end() && it->first == gizmo) {
        it->second();
    }
}
}


void MouseDragDetector::cancel_drag_event()
{
    switch (m_state) {
    case DragState::NoDrag:
        return;
    case DragState::Dragging:
        call_cancel(m_dragging_gizmo, m_on_cancels);
        m_state = DragState::NoDrag;
        return;
    case DragState::StartWeWillSee:
        m_state = DragState::NoDrag;
    }
}

bool MouseDragDetector::can_start_drag()
{
    switch (m_state) {
    case DragState::NoDrag:
        return !m_right_down && !m_middle_down;
    case DragState::Dragging:
        log_weird_state("Try to start drag during dragging. Second left mouse button in row");
        cancel_drag_event();
        return false;
    case DragState::StartWeWillSee:
        log_weird_state("Try to start drag during draging condition");
        m_state = DragState::NoDrag;
        return false;
    default:
        log_weird_state("Undefined DragState in can start funtion.");
        return false;
    }
}

std::string to_string(DragState state)
{
    switch (state) {
    case DragState::NoDrag:
        return "no drag";
    case DragState::Dragging:
        return "dragging";
    case DragState::StartWeWillSee:
        return "start_we_will_see";
    default:
        return "undefined";
    }
}

} // namespace Slic3r::App::Scene
