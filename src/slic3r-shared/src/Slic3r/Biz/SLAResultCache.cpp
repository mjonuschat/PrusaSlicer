#include "Slic3r/Biz/SLAResultCache.hpp"
#include "Slic3r/Assert.hpp"
#include "fmt/ostream.h"

using namespace Slic3r::Biz;
using namespace Slic3r::Biz::Slicing;

SLAResultOptRef SLAResultCache::get_result(const SlicingId& id) const
{
    auto it = m_results.find(id);
    if (it == m_results.end()) {
        SPDLOG_WARN("{}: no data", fmt::streamed(id));
        return std::nullopt;
    }
    return it->second;
}

namespace {
// Return true when cache was updated with new value to invoke listeners
void update_cache(SLAResultCache::Cache& cache, const SlicingId& id, SLAResult&& data)
{
    switch (data.type) {
    // initialize OR rewrite cache entry
    case Sla::ResultType::Slices: cache[id] = std::move(data); break;
    case Sla::ResultType::Files: { // extend cache entry
        auto cache_it = cache.find(id);
        ASSERT(cache_it != cache.end(), "cache must be already filled");
        cache_it->second.files = std::move(data.files);
        break;
    }
    default:
        PANIC("Unsupported SLAResult type in cache");
    }
}
} // namespace

void SLAResultCache::on_sla_result_changed(const SlicingId& id, SLAResult&& result)
{
    update_cache(m_results, id, std::move(result));
    invoke_listeners<ISLAResultCacheChangedListener>([&id](auto* listener) {
        listener->on_sla_result_cache_changed(id); });
    SPDLOG_INFO("{}: update sla ", fmt::streamed(id));
}

void SLAResultCache::on_remove_bed(const SlicingId& id){
    m_results.erase(id);
    SPDLOG_INFO("{}: remove bed", fmt::streamed(id));
}
