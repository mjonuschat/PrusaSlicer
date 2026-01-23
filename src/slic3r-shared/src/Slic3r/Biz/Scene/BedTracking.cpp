#include "Slic3r/Biz/Scene/BedTracking.hpp"

#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Domain/Model.hpp"

namespace Slic3r::Biz {

namespace {
bool remove_instance(Domain::ModelInstanceList& instances, Domain::ModelInstance* inst)
{
    auto it = std::find(instances.begin(), instances.end(), inst);
    if (it == instances.end())
        return false;
    instances.erase(it);
    return true;
}
} // anonymous namespace

void remove_instance_from_bed(Domain::Project& project, Domain::ModelInstance* model_instance, BedTrackingChanges& changes)
{
    if (remove_instance(project.unplaced_model_instances(), model_instance)) {
        changes.unplaced_instances_updated = true;
        for (auto& cc : project.config_containers()) {
            for (auto& bi : cc->bed_instances()) {
                if (remove_instance(bi->colliding_instances, model_instance))
                    --changes.colliding_instances_updated_count;
            }
        }
        return;
    }
    for (auto& cc : project.config_containers())
        for (auto& bi : cc->bed_instances())
            if (remove_instance(bi->model_instances, model_instance)) {
                changes.updated_beds.insert(Domain::BedRef{cc->id().id, bi->id().id});
                return;
            }
}

std::tuple<Domain::ConfigContainer*, Domain::BedInstance*, Algorithms::Bed::BedContainmentState>
find_bed_instance_for_bounds(Domain::Project& project, const Domain::BoundingBox3d& bounds)
{
    for (auto& cc : project.config_containers()) {
        for (auto& bi : cc->bed_instances()) {
            Algorithms::Bed::BedContainmentState state = Algorithms::Bed::contains_3d(*bi, bounds);
            if (state == Algorithms::Bed::BedContainmentState::Inside || state == Algorithms::Bed::BedContainmentState::Colliding)
                return std::make_tuple(cc.get(), bi.get(), state);
        }
    }
    return std::make_tuple(nullptr, nullptr, Algorithms::Bed::BedContainmentState::Outside);
}

Domain::BoundingBox3d BedTracking::get_instance_bb(const Domain::Project& project, const Domain::ModelInstance& inst)
{
    // This function calculates an instance's 3D bounding box, using a cache to boost performance.
    // The cache is invalidated if the instance's transform, its object's internal volume transforms,
    // or any volume's ID or type are modified.
    // It reuses cached geometry for instances sharing the same rotation to further optimize.
    // The cache also periodically removes entries for instances that no longer exist.

    CacheObjectEntry& obj_cache = m_cache[inst.get_object()->id().id];
    
    bool obj_cache_valid = true;
    if (obj_cache.vol_data.size() != inst.get_object()->volumes.size())
        obj_cache_valid = false;
    if (obj_cache_valid) {
        for (size_t i=0; i<obj_cache.vol_data.size(); ++i) {
            if (obj_cache.vol_data[i].id != inst.get_object()->volumes[i]->id().id
             || obj_cache.vol_data[i].type != inst.get_object()->volumes[i]->type()) {
                obj_cache_valid = false;
                break;
            }
            if (obj_cache.vol_data[i].type == Domain::ModelVolumeType::MODEL_PART
             && ! obj_cache.vol_data[i].trafo.get_matrix().isApprox(inst.get_object()->volumes[i]->get_transformation().get_matrix())) {
                obj_cache_valid = false;
                break;
            }
        }
    }
    if (! obj_cache_valid) {
        obj_cache.instances.clear();
        obj_cache.vol_data.clear();
        for (const Domain::ModelVolume* mv : inst.get_object()->volumes)
            obj_cache.vol_data.emplace_back(mv->get_transformation(), mv->id().id, mv->type());
    }

    CacheInstanceEntry& cache_entry = obj_cache.instances[inst.id().id];
    const Domain::Transformation& trafo = inst.get_transformation();

    // Try to use the cache
    bool cache_used = false;
    if (cache_entry.cached_bb.defined) {
        if (cache_entry.inst_trafo.get_rotation().isApprox(trafo.get_rotation()) &&
            cache_entry.inst_trafo.get_scaling_factor().isApprox(trafo.get_scaling_factor()) &&
            cache_entry.inst_trafo.get_mirror().isApprox(trafo.get_mirror())) {
            // Rotation, scale and mirror are the same as before. We can just translate the bounding box itself.
            Domain::Vec3d shift = trafo.get_offset() - cache_entry.inst_trafo.get_offset();
            cache_entry.cached_bb = Algorithms::BoundingBox::translated(cache_entry.cached_bb, shift);
            cache_entry.inst_trafo = trafo;
            cache_used = true;
        }
    }

    if (! cache_used) {
        // We must calculate the transformed bounding box in this case.
        auto bb = Algorithms::ModelObject::instance_bounding_box(*inst.get_object(), inst, Domain::SINKING_Z_THRESHOLD);
        cache_entry = { trafo, bb };
    }

    // Clear the cache every now and then.
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_last_cache_clear_time).count() > 20) {
        m_last_cache_clear_time = std::chrono::steady_clock::now();
        // Get list of all currently existing instances.
        std::vector<size_t> existing_instances;
        for (const Domain::ModelObject* mo : project.model().objects) {
            for (const Domain::ModelInstance* mi : mo->instances)
                existing_instances.emplace_back(mi->id().id);
        }
        std::ranges::sort(existing_instances);

        // Erase all records for instances which no longer exist.
        for (auto& [obj_id, obj_cache] : m_cache) {
            std::erase_if(obj_cache.instances, [&existing_instances](const auto& item) {
                auto it = std::lower_bound(existing_instances.begin(), existing_instances.end(), item.first);
                return it == existing_instances.end() || *it != item.first;
            });
        }
        // Remove all object entries which have empty instances vector.
        std::erase_if(m_cache, [](const std::pair<size_t, CacheObjectEntry>& item) {
            return item.second.instances.empty();
        });
    }

    return cache_entry.cached_bb;
}

void BedTracking::update_instance_bed_placement(Domain::Project& project, Domain::ModelInstance& inst, BedTrackingChanges& changes)
{
    const Domain::BoundingBox3d& bb = get_instance_bb(project, inst);

    auto [cc, bi, state] = find_bed_instance_for_bounds(project, bb);
    if (bi != nullptr) {
        if (state == Algorithms::Bed::BedContainmentState::Inside) {
            bi->model_instances.push_back(&inst);
            changes.updated_beds.insert(Domain::BedRef{ cc->id().id, bi->id().id });
            return;
        }
        else {
            bi->colliding_instances.push_back(&inst);
            ++changes.colliding_instances_updated_count;
        }
    }
    project.unplaced_model_instances().push_back(&inst);
    changes.unplaced_instances_updated = true;
}



BedTrackingChanges BedTracking::update_instances_bed_placement(Domain::Project& project)
{
    BedTrackingChanges changes;

    // Clear old tracking
    project.unplaced_model_instances().clear();
    for (auto& cc : project.config_containers())
        for (auto& bi : cc->bed_instances()) {
            auto& insts = bi->model_instances;
            if (!insts.empty()) {
                insts.clear();
                changes.updated_beds.insert(Domain::BedRef{cc->id().id, bi->id().id});
            }
        }

    // Build new tracking
    for (auto* o : project.model().objects)
        for (auto* inst : o->instances)
            update_instance_bed_placement(project, *inst, changes);

    return changes;
}

BedTrackingChanges BedTracking::update_instances_bed_placement(Domain::Project& project, const Domain::ElementRefs& instances, bool remove_original_links)
{
    BedTrackingChanges changes;
    for (const auto& e : instances) {
        if (e.is_wipe_tower()) {
            const std::size_t bed_instance_id{e.wipe_tower_id.bed_instance_id};
            const Domain::ConfigContainer* config_container{
                project.find_config_container_by_bed_instance_id(bed_instance_id)
            };
            if (config_container == nullptr) {
                continue;
            }
            changes.updated_beds.insert(Domain::BedRef{config_container->id().id, bed_instance_id});
            continue;
        }
        auto* inst = DEBUG_ASSERT_VAL(project.find_instance_by_id(e.object_id, e.instance_id));
        if (remove_original_links)
            remove_instance_from_bed(project, inst, changes);
        if (inst == nullptr)
            continue;
        update_instance_bed_placement(project, *inst, changes);
    }
    return changes;
}

BedTrackingChanges
BedTracking::update_instances_bed_placement(Domain::Project& project, const Domain::ModelInstanceList& instances, bool remove_original_links)
{
    BedTrackingChanges changes;
    for (auto* inst : instances) {
        if (remove_original_links)
            remove_instance_from_bed(project, inst, changes);
        update_instance_bed_placement(project, *inst, changes);
    }
    return changes;
}
} // namespace Slic3r::Biz
