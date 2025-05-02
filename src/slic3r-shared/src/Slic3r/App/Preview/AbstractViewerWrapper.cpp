#include "Slic3r/App/Preview/AbstractViewerWrapper.hpp"
#include "Slic3r/App/libvgcode/AbstractViewer.hpp"

namespace Slic3r::App::Preview {

bool AbstractViewerWrapper::has_data() const 
{
    return viewer().layers_count() > 0;
}

const libvgcode::Lights& AbstractViewerWrapper::lights() const
{
    return viewer().lights();
}

void AbstractViewerWrapper::set_lights(const libvgcode::Lights& lights)
{
    viewer().set_lights(lights);
}

const libvgcode::Lights& AbstractViewerWrapper::default_lights() const
{
    return viewer().default_lights();
}

DoubleSliderForLayers *AbstractViewerWrapper::double_slider_layers() const
{
    return m_slider_layers;
}

} // namespace Slic3r::App::Preview
