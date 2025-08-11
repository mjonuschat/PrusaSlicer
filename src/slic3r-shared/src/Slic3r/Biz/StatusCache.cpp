#include "Slic3r/Biz/StatusCache.hpp"

namespace Slic3r::Biz {

void StatusCache::on_status_changed(const Slicing::Status status, const Domain::SlicingId id)
{
    if (status == Slicing::Status::Removed) {
        m_statuses.erase(id);
    } else {
        m_statuses[id] = status;
    }
    invoke_listeners<IStatusCacheChangedListener>([&](auto* listener){
        listener->on_status_cache_changed(id);
    });
}

std::optional<Slicing::Status> StatusCache::get_status(const Domain::SlicingId id) const
{
    if(!m_statuses.contains(id)) {
        return std::nullopt;
    }
    return m_statuses.at(id);
}

} // namespace Slic3r::Biz
