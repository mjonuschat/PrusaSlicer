#pragma once

#include <map>
#include <functional>
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/libpgcode/ProcessorResult.hpp"

namespace Slic3r::Biz {

struct IFDMResultCacheChangedListener
{
    virtual ~IFDMResultCacheChangedListener() = default;
    virtual void on_fdm_result_cache_changed(const Slicing::SlicingId id) = 0;
};

class FDMResultCache :
    public Slicing::IFDMResultListener,
    public WithListeners<IFDMResultCacheChangedListener>
{
public:
    const Slicing::FDMResult& get_result(const Slicing::SlicingId id) const;

    void on_fdm_result_changed(
        Slicing::FDMResult&& result,
        const Slicing::SlicingId id
    ) override;

private:
    std::map<Slicing::SlicingId, Slicing::FDMResult> m_results;
};
} // namespace Slic2r::Biz::FDMResultCache
