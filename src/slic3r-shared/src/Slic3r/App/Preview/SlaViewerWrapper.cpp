#include "Slic3r/App/Preview/SlaViewerWrapper.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/Biz/libpgcode/Utils.hpp"
#include "Slic3r/Domain/Constants.hpp"

#include <iostream>

using namespace Slic3r::Biz::libpgcode;
using namespace Slic3r::App::libvgcode;
using namespace Slic3r::Biz;

namespace Slic3r::App::Preview {

SlaViewerWrapper::~SlaViewerWrapper() = default;

bool SlaViewerWrapper::init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory)
{
    try {
        m_viewer.init(device, scene, data_factory);
        return true;
    }
    catch (const std::exception& e) {
        std::cout << e.what();
        return false;
    }
}

void SlaViewerWrapper::render_scene()
{
    m_viewer.render();
}

void SlaViewerWrapper::render_imgui() 
{
}

bool SlaViewerWrapper::set_settings(const ViewerWrapperBaseSettings& settings)
{
    m_settings = settings;

    try {
        m_slider_layers = Yoga::Passthrough(std::make_unique<DoubleSliderForLayers>());
        m_slider_layers->show_ruler(m_settings.slider_layers_show_ruler, m_settings.slider_layers_show_ruler_bg);
        m_slider_layers->show_estimated_times(m_settings.slider_layers_show_estimated_times);
        // set layers slider callbacks
        m_slider_layers->set_on_thumb_move_callback(std::bind(&SlaViewerWrapper::on_slider_layers_scroll_changed, this));

        return true;
    }
    catch (const std::exception& e) {
        std::cout << e.what();
        return false;
    }
}

void SlaViewerWrapper::reset()
{
    m_viewer.reset();
}

void SlaViewerWrapper::load(SlaViewerWrapperInputData&& wrapper_data, const std::vector<float>& layers_zs, const std::vector<float>& layers_times)
{
    m_loading = true;

    m_data = std::move(wrapper_data);

    m_viewer.load(layers_zs, layers_times);

    update_slider_layers();

    m_loading = false;
}

// void SlaViewerWrapper::render_legend(Render::ImguiRender* imgui_render)
// {
//     static std::string msg = _u8L("No data available");

//     if (!has_data()) {
//         ImVec2 msg_size = ImGui::CalcTextSize(msg.c_str());
//         ImVec2 available_size = ImGui::GetContentRegionAvail();
//         if (msg_size.x < available_size.x && msg_size.y < available_size.y) {
//             ImVec2 pos = ImGui::GetCurrentWindow()->DC.CursorPos + (available_size - msg_size) * 0.5f;
//             ImGui::RenderText(pos, msg.c_str());
//         }
//     }
//     else {
//         // ToDo Render SLA legend
//     }
// }

void SlaViewerWrapper::set_layers_range(Interval::value_type min, Interval::value_type max)
{
    m_viewer.set_layers_range(min, max);
}

void SlaViewerWrapper::update_slider_layers()
{
    // !!! Code duplication

    // Save the initial slider span.
    float z_low = m_slider_layers->lower_value();
    float z_high = m_slider_layers->higher_value();
    bool was_empty = m_slider_layers->max_pos() == 0;

    std::vector<float> layers_zs = m_viewer.layers_zs();

    bool force_sliders_full_range = was_empty || layers_zs.empty() || std::abs(layers_zs.back() - m_slider_layers->max_value()) > Domain::EPSILON;
    bool snap_to_min = force_sliders_full_range || m_slider_layers->is_lower_at_min();
    bool snap_to_max = force_sliders_full_range || m_slider_layers->is_higher_at_max();

    int max_pos = layers_zs.empty() ? 0 : int(layers_zs.size()) - 1;

    int idx_low = 0;
    int idx_high = max_pos;
    if (!layers_zs.empty()) {
        if (!snap_to_min) {
            int idx_new = DoubleSliderForLayers::find_close_layer_idx(layers_zs, z_low, float(Domain::EPSILON));
            if (idx_new != -1)
                idx_low = idx_new;
        }
        if (!snap_to_max) {
            int idx_new = DoubleSliderForLayers::find_close_layer_idx(layers_zs, z_high, float(Domain::EPSILON));
            if (idx_new != -1)
                idx_high = idx_new;
        }
    }

    m_slider_layers->set_slider_values(std::move(layers_zs));
    m_slider_layers->force_ruler_update();
    assert(m_slider_layers->min_pos() == 0);
    m_slider_layers->freeze();
    m_slider_layers->set_max_pos(max_pos);
    m_slider_layers->set_selection_span(idx_low, idx_high);

    if (!m_data.keep_layers_times)
        m_slider_layers->set_layers_times(m_viewer.layers_estimated_times(), m_viewer.estimated_time());

    m_slider_layers->thaw();
}

void SlaViewerWrapper::update_view_visible_range(size_t first, size_t last)
{
    m_viewer.set_view_visible_range(first, last);
}

void SlaViewerWrapper::on_slider_layers_scroll_changed()
{
    if (m_slider_layers->is_visible()) {
        set_layers_range(uint32_t(m_slider_layers->lower_pos()), uint32_t(m_slider_layers->higher_pos()));
        if (m_settings.cb_slider_layers_on_thumb_move != nullptr)
            m_settings.cb_slider_layers_on_thumb_move();
    }
}

} // namespace Slic3r::App::Preview
