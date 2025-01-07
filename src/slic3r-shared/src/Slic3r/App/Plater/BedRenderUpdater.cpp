#include "Slic3r/App/Plater/BedRenderUpdater.hpp"
#include "Slic3r/App/Plater/BedNodeTag.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Project.hpp"

namespace Slic3r::App::Plater {

void BedRenderUpdater::update_materials(Render::Device& device, const Domain::Project& project)
{
    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::Grid ||
                tag->type == BedElementType::Contour ||
                tag->type == BedElementType::PrintVolume ||
                tag->type == BedElementType::Model) {
                const Domain::Bed* bed = project.bed_container().bed(tag->bed_id);
                DEBUG_ASSERT(bed != nullptr);
                const Domain::BedInstance* inst = bed->instance(tag->instance_id);
                DEBUG_ASSERT(inst != nullptr);
                if (inst->active())
                    n.remove_material_override();
                else {
                    std::string shader_name = (tag->type == BedElementType::Model) ? "gouraud_light" : "flat";
                    ColorRGBA color;
                    switch (tag->type)
                    {
                    case BedElementType::Grid:         { color = DISABLED_BED_GRID_COLOR; break; }
                    case BedElementType::Contour:
                    case BedElementType::PrintVolume:  { color = DISABLED_BED_CONTOUR_COLOR; break; }
                    case BedElementType::PlateDefault: { color = DISABLED_BED_PLATE_COLOR; break; }
                    default:                           { color = DISABLED_BED_MODEL_COLOR; break; }
                    }
                    auto material = Render::Material{}
                        .set_shader(device.context().shader_manager().get_shader(shader_name))
                        .set_uniform("uniform_color", color)
                        .set_transparent(color.a() < 1.0f);
                    n.set_material_override(material);
                }
            }
        }
    }, true);
}

void BedRenderUpdater::update_positions(const Domain::Project& project)
{
    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Undefined) {
                const Domain::Bed* bed = project.bed_container().bed(tag->bed_id);
                DEBUG_ASSERT(bed != nullptr);
                const Domain::BedInstance* inst = bed->instance(tag->instance_id);
                DEBUG_ASSERT(inst != nullptr);
                n.set_world_transform(inst->matrix().matrix());
            }
        }
    }, true);
}

void BedRenderUpdater::update_elements_state(const Domain::Project& project)
{
    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Contour || tag->type == BedElementType::PrintVolume) {
                const Domain::Bed* bed = project.bed_container().bed(tag->bed_id);
                DEBUG_ASSERT(bed != nullptr);
                const Domain::BedInstance* inst = bed->instance(tag->instance_id);
                DEBUG_ASSERT(inst != nullptr);
                if (tag->type == BedElementType::Contour)
                    n.set_enabled(inst->contour_enabled());
                else if (tag->type == BedElementType::PrintVolume)
                    n.set_enabled(inst->print_volume_enabled());
            }
        }
    }, true);
}

void BedRenderUpdater::camera_updated(const Scene::Camera& cam)
{
    // turn beds' plate and model visibility on/off in dependence of camera position/orientation
    Scene::CameraProjectionType cam_type = cam.cam_projection().type();
    bool show_bottom = (cam_type == Scene::CameraProjectionType::Perspective) ? cam.position().z() < 0.0 : cam.forward().z() >= 0.0;
    auto& scene = m_scene_provider.scene();
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::Model)
                n.set_enabled(!show_bottom);
        }
    }, true);
}

} // namespace Slic3r::App::Plater