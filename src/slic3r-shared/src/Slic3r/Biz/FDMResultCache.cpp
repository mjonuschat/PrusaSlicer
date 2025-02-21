#include "Slic3r/Biz/FDMResultCache.hpp"
#include "fmt/ostream.h"

namespace Slic3r::Biz {

const Slicing::FDMResult& FDMResultCache::get_result(const Slicing::SlicingId id) const {
    return m_results.at(id);
}

void FDMResultCache::on_fdm_result_changed(
    std::shared_ptr<Slicing::FDMResult> result,
    const Slicing::SlicingId id
)
{
    m_results[id] = std::move(*result);
    invoke_listeners<IFDMResultCacheChangedListener>([&](auto* listener) {
        listener->on_fdm_result_cache_changed(id);
    });

    if (m_results[id].moves.empty()) {
        m_results.erase(id);
        SPDLOG_INFO("{}: cleared", fmt::streamed(id));
    } else {
        SPDLOG_INFO("{}: updated", fmt::streamed(id));
    }
}

} // namespace Slic2r::Biz::FDMResultCache
