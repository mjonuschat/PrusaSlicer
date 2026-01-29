#include "Slic3r/App/Preview/AbstractViewerWrapper.hpp"
#include "Slic3r/App/libvgcode/AbstractViewer.hpp"

namespace Slic3r::App::Preview {

bool AbstractViewerWrapper::has_data() const 
{
    return viewer().layers_count() > 0;
}

const Scene::Lighting& AbstractViewerWrapper::lights() const
{
    return viewer().lights();
}

void AbstractViewerWrapper::set_lights(const Scene::Lighting& lights)
{
    viewer().set_lights(lights);
}

std::unique_ptr<DoubleSliderForLayers> AbstractViewerWrapper::unload_double_slider_layers()
{
    return m_slider_layers.release();
}

void AbstractViewerWrapper::set_layers_slider_base_flags(LayersSliderBaseFlags flags)
{
    if (m_slider_layers.get()) {
        m_slider_layers->show_ruler(flags.show_ruler, flags.show_ruler_bg);
        m_slider_layers->show_estimated_times(flags.show_estimated_times);
    }
}

} // namespace Slic3r::App::Preview
