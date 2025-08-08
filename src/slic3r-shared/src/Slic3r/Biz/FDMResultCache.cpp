#include "Slic3r/Biz/FDMResultCache.hpp"
#include "fmt/ostream.h"

namespace Slic3r::Biz {

std::optional<FDMResultRef> FDMResultCache::get_result(const Domain::SlicingId id) const
{
    if (!m_results.contains(id)) {
        return std::nullopt;
    }
    return m_results.at(id);
}

void FDMResultCache::on_fdm_result_changed(
    Slicing::FDMResult&& result,
    const Domain::SlicingId id
)
{
    m_results[id] = std::move(result);

    if (m_results[id].const_moves()->empty()) {
        m_results.erase(id);
        SPDLOG_INFO("{}: cleared", fmt::streamed(id));
    } else {
        SPDLOG_INFO("{}: updated", fmt::streamed(id));
    }

    invoke_listeners<IFDMResultCacheChangedListener>([&](auto* listener) {
        listener->on_fdm_result_cache_changed(id);
    });
}

} // namespace Slic2r::Biz::FDMResultCache
