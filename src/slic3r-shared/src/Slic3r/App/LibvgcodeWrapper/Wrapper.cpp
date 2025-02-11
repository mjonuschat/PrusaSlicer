#include "Slic3r/App/LibvgcodeWrapper/Wrapper.hpp"
#include "Slic3r/App/LibvgcodeWrapper/WrapperImpl.hpp"

namespace Slic3r::App::LibvgcodeWrapper {

Wrapper::Wrapper()
    : m_impl(new WrapperImpl)
{
}

Wrapper::~Wrapper() = default;

bool Wrapper::init(const WrapperSettings& settings)
{
    return m_impl->init(settings);
}

void Wrapper::shutdown()
{
    m_impl->shutdown();
}

void Wrapper::reset()
{
    m_impl->reset();
}

} // namespace Slic3r::App::LibvgcodeWrapper
