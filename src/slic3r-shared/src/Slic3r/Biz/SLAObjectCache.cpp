#include "Slic3r/Biz/SLAObjectCache.hpp"
#include "fmt/ostream.h"

using namespace Slic3r::Biz;
using namespace Slic3r::Biz::Slicing;
using Slic3r::Biz::Slicing::Sla::Object;

SLAObjectOptRef SLAObjectCache::get_instance(const SLAObjectCache::Key& key) const
{
    auto it = m_objects.find(key);
    if (it == m_objects.end()) {
        SPDLOG_WARN("{}: no instance for key", fmt::streamed(key.first));
        return std::nullopt;
    }    
    return it->second;
}

void SLAObjectCache::on_sla_object_changed(const SlicingId& id, Object&& object)
{
    assert(object.object_id != 0);
    if (object.object_id == 0)
        return; // invalid instance id
    
    Key key(id, object.object_id);
    m_objects[key] = std::move(object);
    invoke_listeners<ISLAObjectCacheChangedListener>([&id](auto* listener) {
        listener->on_sla_object_cache_changed(id);
    });
    SPDLOG_INFO("{}: sla object updated", fmt::streamed(id));
}

void SLAObjectCache::on_model_update(
    const Slicing::SlicingId& id, 
    const std::vector<Domain::ObjectID>& object_ids)
{
    size_t count = std::erase_if(m_objects, [&id, &object_ids](const auto& pair) {
        return pair.first.first == id &&
            std::find(object_ids.begin(), object_ids.end(), pair.second.object_id)
            == object_ids.end(); 
    });
    if (count!=0)
        SPDLOG_INFO("{}: sla object cache updated", fmt::streamed(id));
}

void SLAObjectCache::on_remove_bed(const Slicing::SlicingId& id) {
    std::erase_if(m_objects, [&id](const auto& pair) { return pair.first.first == id; });
}
