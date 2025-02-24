#include "Slic3r/Biz/Scene/BedTracking.hpp"

#include "Slic3r/Domain/Project.hpp"

#include <libslic3r/Model.hpp>

namespace Slic3r::Biz {

namespace {
bool remove_instance(Domain::ModelInstanceList& instances, ModelInstance* inst)
{
    auto it = std::find(instances.begin(), instances.end(), inst);
    if (it == instances.end())
        return false;
    instances.erase(it);
    return true;
}
}

void remove_instance_from_bed(Domain::Project& project, ModelInstance* model_instance, BedTrackingChanges& changes)
{
    if (remove_instance(project.unplaced_model_instances(), model_instance)) {
        changes.unplaced_instances_updated = true;
        return;
    }
    for (auto& cc : project.config_containers())
        for (auto& bi : cc->bed_instances())
            if (remove_instance(bi->model_instances(), model_instance)) {
                changes.updated_beds.insert(Domain::BedRef{cc->id().id, bi->id().id});
                return;
            }
}



std::pair<Domain::ConfigContainer*, Domain::BedInstance*> find_bed_instance_for_bounds(Domain::Project& project, const BoundingBoxf& bounds)
{
    for (auto& cc : project.config_containers())
        for (auto& bi : cc->bed_instances())
            if (bi->contains(bounds))
                return std::make_pair(cc.get(), bi.get());
    return std::make_pair(nullptr, nullptr);
}

void update_instance_bed_placement(Domain::Project& project, ModelInstance& inst, BedTrackingChanges& changes)
{

    const auto bb = to_2d(inst.get_object()->instance_bounding_box(inst));
    if (auto [cc, bi] = find_bed_instance_for_bounds(project, bb); bi != nullptr) {
        bi->model_instances().push_back(&inst);
        changes.updated_beds.insert(Domain::BedRef{cc->id().id, bi->id().id});
    }
    else {
        project.unplaced_model_instances().push_back(&inst);
        changes.unplaced_instances_updated = true;
    }
}

BedTrackingChanges update_instances_bed_placement(Domain::Project& project)
{
    BedTrackingChanges changes;

    // Clear old tracking
    project.unplaced_model_instances().clear();
    for (auto& cc : project.config_containers())
        for (auto& bi : cc->bed_instances()) {
            auto& insts = bi->model_instances();
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

BedTrackingChanges update_instances_bed_placement(Domain::Project& project, const Domain::ElementRefs& instances, bool remove_original_links)
{
    BedTrackingChanges changes;
    for (const auto& e : instances) {
        auto* inst = DEBUG_ASSERT_VAL(project.find_instance_by_id(e.object_id, e.instance_id));
        if (remove_original_links)
            remove_instance_from_bed(project, inst, changes);
        if (inst == nullptr)
            continue;
        update_instance_bed_placement(project, *inst, changes);
    }
    return changes;
}

BedTrackingChanges update_instances_bed_placement(Domain::Project& project, const Domain::ModelInstanceList& instances, bool remove_original_links)
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
