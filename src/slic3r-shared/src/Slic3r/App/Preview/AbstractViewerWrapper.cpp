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

} // namespace Slic3r::App::Preview
