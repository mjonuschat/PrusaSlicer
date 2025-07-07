#include "Slic3r/Biz/Algorithms/Model.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

#include <boost/filesystem.hpp>

using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::Biz::Algorithms::Model {

void translate(Domain::Model& model, const double x, const double y, const double z)
{
    for (Domain::ModelObject* o : model.objects) {
        ModelObject::translate(*o, x, y, z);
    }
}

Domain::ModelObject* add_object(Domain::Model* model,const char* name, const char* path, const Domain::TriangleMesh& mesh)
{
    Domain::ModelObject* new_object = new Domain::ModelObject(model);
    model->objects.push_back(new_object);
    new_object->name = name;
    new_object->input_file = path;
    Domain::ModelVolume* new_volume = ModelObject::add_volume(new_object, mesh);
    new_volume->name = name;
    new_volume->source.input_file = path;
    new_volume->source.object_idx = (int) model->objects.size() - 1;
    new_volume->source.volume_idx = (int) new_object->volumes.size() - 1;
    new_object->invalidate_bounding_box();
    return new_object;
}

Domain::ModelObject* add_object(Domain::Model* model,const char* name, const char* path, Domain::TriangleMesh&& mesh)
{
    Domain::ModelObject* new_object = new Domain::ModelObject(model);
    model->objects.push_back(new_object);
    new_object->name = name;
    new_object->input_file = path;
    Domain::ModelVolume* new_volume = ModelObject::add_volume(new_object, std::move(mesh));
    new_volume->name = name;
    new_volume->source.input_file = path;
    new_volume->source.object_idx = (int) model->objects.size() - 1;
    new_volume->source.volume_idx = (int) new_object->volumes.size() - 1;
    new_object->invalidate_bounding_box();
    return new_object;
}

// This returns the bounding box of the *transformed* instances.
Domain::BoundingBox3d bounding_box_approx(const Domain::Model& model)
{
    Domain::BoundingBox3d bb;
    for (Domain::ModelObject* o : model.objects) {
        bb = BoundingBox::merge(bb, ModelObject::bounding_box_approx(*o));
    }

    return bb;
}

Domain::BoundingBox3d bounding_box_exact(const Domain::Model& model)
{
    Domain::BoundingBox3d bb;
    for (Domain::ModelObject* o : model.objects) {
        bb = BoundingBox::merge(bb, ModelObject::bounding_box_exact(*o));
    }

    return bb;
}

// Propose a filename including path derived from the ModelObject's input path.
// If object's name is filled in, use the object name, otherwise use the input name.
std::string propose_export_file_name_and_path(const Domain::Model& model)
{
    std::string input_file;
    for (const Domain::ModelObject* model_object : model.objects)
        for (Domain::ModelInstance* model_instance : model_object->instances)
            if (model_instance->is_printable()) {
                input_file = ModelObject::get_export_filename(*model_object);

                if (!input_file.empty()) {
                    goto end;
                }

                // Other instances will produce the same name, skip them.
                break;
            }
end:
    return input_file;
}

std::string propose_export_file_name_and_path(const Domain::Model& model, const std::string& new_extension)
{
    return boost::filesystem::path(Model::propose_export_file_name_and_path(model)).replace_extension(new_extension).string();
}

bool center_instances_around_point(Domain::Model& model, const Domain::Vec2d& point)
{
    Domain::BoundingBox3d bb;
    for (Domain::ModelObject* o : model.objects) {
        for (size_t i = 0; i < o->instances.size(); ++i) {
            bb = BoundingBox::merge(bb, ModelObject::instance_bounding_box(*o, i, false));
        }
    }

    Domain::Vec2d shift2 = point - Point::to_2d(BoundingBox::center(bb));
    if (std::abs(shift2(0)) < Domain::EPSILON && std::abs(shift2(1)) < Domain::EPSILON)
        return false; // No significant shift, don't do anything.

    Domain::Vec3d shift3 = Domain::Vec3d(shift2(0), shift2(1), 0.0);
    for (Domain::ModelObject* o : model.objects) {
        for (Domain::ModelInstance* i : o->instances) {
            i->set_offset(i->get_offset() + shift3);
        }

        o->invalidate_bounding_box();
    }

    return true;
}

void duplicate_objects_grid(Domain::Model& model, const size_t x, const size_t y, const double dist)
{
    if (model.objects.size() > 1)
        throw "Grid duplication is not supported with multiple objects";

    if (model.objects.empty())
        throw "No objects!";

    Domain::ModelObject* object = model.objects.front();
    object->clear_instances();

    Domain::Vec3d ext_size = BoundingBox::sizes(ModelObject::bounding_box_exact(*object)) + dist * Domain::Vec3d::Ones();

    for (size_t x_copy = 1; x_copy <= x; ++x_copy) {
        for (size_t y_copy = 1; y_copy <= y; ++y_copy) {
            Domain::ModelInstance* instance = object->add_instance();
            instance->set_offset(Domain::Vec3d(ext_size.x() * (double) (x_copy - 1), ext_size.y() * (double) (y_copy - 1), 0.));
        }
    }
}

void adjust_min_z(Domain::Model& model)
{
    if (model.objects.empty())
        return;

    if (Model::bounding_box_exact(model).min.z() < 0.) {
        for (Domain::ModelObject* obj : model.objects) {
            if (obj != nullptr) {
                double obj_min_z = obj->min_z();
                if (obj_min_z < 0.) {
                    obj->translate_instances(Domain::Vec3d(0., 0., -obj_min_z));
                }
            }
        }
    }
}

// Flattens everything to a single mesh.
Domain::TriangleMesh flatten_to_mesh(const Domain::Model& model)
{
    Domain::TriangleMesh mesh;
    for (const Domain::ModelObject* o : model.objects) {
        mesh.merge(ModelObject::mesh(*o));
    }

    return mesh;
}

void print_info(const Domain::Model& model)
{
    for (const Domain::ModelObject* o : model.objects) {
        ModelObject::print_info(*o);
    }
}

} // namespace Slic3r::Biz::Algorithms::Model
