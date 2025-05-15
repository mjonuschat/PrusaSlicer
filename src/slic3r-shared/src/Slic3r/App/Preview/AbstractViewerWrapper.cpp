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

DoubleSliderForLayers *AbstractViewerWrapper::double_slider_layers() const
{
    return m_slider_layers;
}

} // namespace Slic3r::App::Preview

