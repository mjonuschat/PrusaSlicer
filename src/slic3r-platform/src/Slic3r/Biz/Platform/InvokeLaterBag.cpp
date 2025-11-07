#include "Slic3r/Biz/Platform/InvokeLaterBag.hpp"

namespace Slic3r::Biz {

InvokeLaterBag::~InvokeLaterBag()
{
    for (const auto& func : m_to_invoke) {
        func();
    }
}

void InvokeLaterBag::add(Func&& func)
{
    m_to_invoke.emplace_back(std::forward<Func>(func));
}

} // namespace Slic3r::Biz
