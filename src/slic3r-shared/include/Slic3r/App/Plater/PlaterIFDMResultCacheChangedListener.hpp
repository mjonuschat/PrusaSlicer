#pragma once

#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/Slicing/BackgroundProcess.hpp"

namespace Slic3r::App::Plater {

class PlaterFDMResultCacheChangedListener : public Biz::IFDMResultCacheChangedListener
{
public:
	typedef std::function<void(/*const Biz::Slicing::SlicingId id*/)> CallbackFn;

	PlaterFDMResultCacheChangedListener(CallbackFn callback, Biz::FDMResultCache& cache)
		: m_callback_fn{callback}
	{
        cache.add_listener<Biz::IFDMResultCacheChangedListener>(this);
    }
	void on_fdm_result_cache_changed(const Biz::Slicing::SlicingId id) override 
    { 
        m_callback_fn(); 
        m_last_id = id;
    }
    const Biz::Slicing::SlicingId get_last_id() const { return m_last_id; }
private:
	CallbackFn m_callback_fn;
    Biz::Slicing::SlicingId m_last_id;
};

} // namespace Slic3r::App::Plater 