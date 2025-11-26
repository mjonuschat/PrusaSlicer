#include "Slic3r/InvokeLaterBag.hpp"

namespace Slic3r {

InvokeLaterBag::~InvokeLaterBag()
{
    invoke_now();
}

void InvokeLaterBag::invoke_now()
{
    for (const auto& func : m_to_invoke) {
        func();
    }
    m_to_invoke.clear();
}

void InvokeLaterBag::add(Func&& func)
{
    m_to_invoke.emplace_back(std::forward<Func>(func));
}

} // namespace Slic3r::Biz
