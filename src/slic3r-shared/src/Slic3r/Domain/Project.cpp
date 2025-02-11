#include "Slic3r/Domain/Project.hpp"

#include "libslic3r/NSVGUtils.hpp"

#include <boost/geometry/index/detail/algorithms/bounds.hpp>
#include <libslic3r/Model.hpp>

namespace Slic3r::Domain {

Project::Project() : m_model(new Model()) {}

void Project::load(const std::string& file_path)
{
    // TODO: implement
    /*
    m_model = std::make_unique<Model>(Model::read_from_file(file_path));
    set_file_name(file_path);
    m_config_containers.clear();
    m_config_containers.emplace_back();
    auto& config_container = m_config_containers.back();
    */
}

const ConfigContainer* Project::find_config_container(size_t id) const
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

ConfigContainer* Project::find_config_container(size_t id)
{
    return find_by_id<ConfigContainer>(m_config_containers, id);
}

const ModelObject* Project::find_object_by_id(size_t id) const
{
    return find_by_id<ModelObject>(m_model->objects, id);
}

const ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

const ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id) const
{
    const auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

ModelObject* Project::find_object_by_id(size_t id)
{
    return find_by_id<ModelObject>(m_model->objects, id);
}

ModelVolume* Project::find_volume_by_id(size_t obj_id, size_t vol_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelVolume>(obj->volumes, vol_id);
}

ModelInstance* Project::find_instance_by_id(size_t obj_id, size_t inst_id)
{
    auto* obj = find_object_by_id(obj_id);
    if (obj == nullptr)
        return nullptr;
    return find_by_id<ModelInstance>(obj->instances, inst_id);
}

const BedInstance* Project::find_bed_instance_by_id(size_t id) const
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

BedInstance* Project::find_bed_instance_by_id(size_t id)
{
    for (const auto& cc : m_config_containers)
        if (auto* bed_inst = find_by_id(cc->bed_instances(), id))
            return bed_inst;
    return nullptr;
}

const Bed* Project::find_bed_by_id(size_t id) const
{
    return find_by_id(m_bed_container.beds(), id);
}

Bed* Project::find_bed_by_id(size_t id)
{
    return find_by_id(m_bed_container.beds(), id);
}


BedInstance* Project::find_bed_instance_for_bounds(const BoundingBoxf& bounds)
{
    for (auto& cc : m_config_containers)
        for (auto& bi : cc->bed_instances())
            if (bi->contains(bounds))
                return bi.get();
    return nullptr;
}

void Project::update_instance_bed_placement(ModelInstance& inst)
{

    const auto bb = to_2d(inst.get_object()->instance_bounding_box(inst));
    if (auto* bi = find_bed_instance_for_bounds(bb))
        bi->model_instances().push_back(&inst);
    else
        m_unplaced_model_instances.push_back(&inst);
}

void Project::update_instances_bed_placement()
{
    // Clear old tracking
    m_unplaced_model_instances.clear();
    for (auto& cc : m_config_containers)
        for (auto& bi : cc->bed_instances())
            bi->model_instances().clear();

    // Build new tracking
    for (auto* o : m_model->objects)
        for (auto* inst : o->instances)
            update_instance_bed_placement(*inst);
}

void Project::update_instances_bed_placement(const ElementRefs& instances, bool remove_original_links)
{
    for (const auto& e : instances) {
        auto* inst = DEBUG_ASSERT_VAL(find_instance_by_id(e.object_id, e.instance_id));
        if (remove_original_links)
            remove_instance_from_bed(inst);
        if (inst == nullptr)
            continue;
        update_instance_bed_placement(*inst);
    }
}

void Project::update_instances_bed_placement(const ModelInstanceList& instances, bool remove_original_links)
{
    for (auto* inst : instances) {
        if (remove_original_links)
            remove_instance_from_bed(inst);
        update_instance_bed_placement(*inst);
    }
}

namespace {
bool remove_instance(ModelInstanceList& instances, ModelInstance* inst)
{
    auto it = std::find(instances.begin(), instances.end(), inst);
    if (it == instances.end())
        return false;
    instances.erase(it);
    return true;
}
}

void Project::remove_instance_from_bed(ModelInstance* model_instance)
{
    if (remove_instance(m_unplaced_model_instances, model_instance))
        return;
    for (auto& cc : m_config_containers)
        for (auto& bi : cc->bed_instances())
            if (remove_instance(bi->model_instances(), model_instance))
                return;
}

} // namespace Slic3r::Domain
