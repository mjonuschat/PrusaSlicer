#include <functional>
#include <libslic3r/Model.hpp>

#include "Slic3r/Biz/Slicing/ModelUtils.hpp"

namespace Slic3r::Biz::Slicing {

using ObjectInstances = std::vector<std::pair<ModelObject*, ModelInstancePtrs>>;
ObjectInstances get_object_instances(const Model& model) {
    ObjectInstances result;

    std::ranges::transform(
        model.objects,
        std::back_inserter(result),
        [](ModelObject *object){
            return std::pair{object, object->instances};
        }
    );

    return result;
}

void restore_object_instances(Model& model, const ObjectInstances &object_instances) {
    ModelObjectPtrs objects;

    std::ranges::transform(
        object_instances,
        std::back_inserter(objects),
        [](const std::pair<ModelObject *, ModelInstancePtrs> &key_value){
            auto [object, instances]{key_value};
            object->instances = std::move(instances);
            return object;
        }
    );

    model.objects = objects;
}

bool contains(const ModelInstance* instance, const Domain::ModelInstanceList& bed_instances)
{
    return std::ranges::find(bed_instances, instance) != bed_instances.end();
}

void remove_instances(
    Model& model,
    const Domain::ModelInstanceList& bed_instances
) {
    for (ModelObject* mo : model.objects) {
        std::erase_if(mo->instances, [&](const ModelInstance* instance) {
            return !contains(instance, bed_instances);
        });
    }

    std::erase_if(model.objects, [](const ModelObject* object) {
        return object->instances.empty();
    });
}

void with_limited_instances(
    Model &model,
    const Domain::ModelInstanceList& bed_instances,
    const std::function<void()> &callable
) {
    const ObjectInstances original_objects{get_object_instances(model)};
    Slic3r::ScopeGuard guard([&]() {
        restore_object_instances(model, original_objects);
    });

    remove_instances(model, bed_instances);
    callable();
}

}
