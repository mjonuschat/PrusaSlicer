#pragma once

#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include <map>

namespace Slic3r::Biz {


struct IStatusCacheChangedListener
{
    virtual ~IStatusCacheChangedListener() = default;
    virtual void on_status_cache_changed(const Domain::SlicingId id) = 0;
};

struct StatusCache :
    public Slicing::IStatusListener,
    public WithListeners<IStatusCacheChangedListener>
{
    void on_status_changed(const Slicing::Status status, const Domain::SlicingId id);
    std::optional<Slicing::Status> get_status(const Domain::SlicingId id) const;

private:
    std::map<Domain::SlicingId, Slicing::Status> m_statuses;
};

} // namespace Slic3r::Biz
