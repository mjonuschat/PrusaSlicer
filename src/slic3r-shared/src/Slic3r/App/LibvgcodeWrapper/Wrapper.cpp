#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperImpl.hpp"

using namespace Slic3r::App::libvgcode;

namespace Slic3r::App::LibvgcodeWrapper {

Wrapper::Wrapper()
    : m_impl(new WrapperImpl)
{
}

Wrapper::~Wrapper() = default;

bool Wrapper::init(App::Render::Device& device, const WrapperSettings& settings)
{
    return m_impl->init(device, settings);
}

void Wrapper::shutdown()
{
    m_impl->shutdown();
}

void Wrapper::reset()
{
    m_impl->reset();
}

void Wrapper::load(WrapperInputData&& wrapper_data, ViewerInputData&& data)
{
    m_impl->load(std::move(wrapper_data), std::move(data));
}

void Wrapper::load_as_sla(WrapperSLAInputData&& wrapper_sla_data)
{
    m_impl->load_as_sla(std::move(wrapper_sla_data));
}

void Wrapper::render(const Transform3f& view_matrix, const Transform3f& projection_matrix,
    const WrapperLayoutData& layout)
{
    m_impl->render(view_matrix, projection_matrix, layout);
}

Biz::libpgcode::UnitsSystem Wrapper::units() const
{
    return m_impl->units();
}

void Wrapper::set_units(Biz::libpgcode::UnitsSystem sys)
{
    m_impl->set_units(sys);
}

bool Wrapper::has_data() const
{
    return m_impl->has_data();
}

void Wrapper::set_legend_visible(bool visible)
{
    m_impl->set_legend_visible(visible);
}

void Wrapper::toggle_legend_visible()
{
    m_impl->toggle_legend_visible();
}

bool Wrapper::is_legend_visible() const
{
    return m_impl->is_legend_visible();
}

void Wrapper::set_gcodewindow_visible(bool visible)
{
    m_impl->set_gcodewindow_visible(visible);
}

void Wrapper::toggle_gcodewindow_visible()
{
    m_impl->toggle_gcodewindow_visible();
}

bool Wrapper::is_gcodewindow_visible() const
{
    return m_impl->is_gcodewindow_visible();
}

const Lights& Wrapper::lights() const
{
    return m_impl->lights();
}

void Wrapper::set_lights(const Lights& lights)
{
    m_impl->set_lights(lights);
}

const Lights& Wrapper::default_lights() const
{
    return m_impl->default_lights();
}

void Wrapper::reset_default_extrusion_roles_colors()
{
    m_impl->reset_default_extrusion_roles_colors();
}

} // namespace Slic3r::App::LibvgcodeWrapper
