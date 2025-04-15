#include "Slic3r/App/Plater/BedRenderUpdater.hpp"
#include "Slic3r/App/Plater/BedNodeTag.hpp"
#include "Slic3r/App/Plater/BedMaterials.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Workbench.hpp"

namespace Slic3r::App::Plater {

void BedRenderUpdater::update_materials()
{
    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr && tag->type != BedElementType::Undefined) {
            DEBUG_ASSERT(m_project != nullptr);
            Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
            DEBUG_ASSERT(cc != nullptr);
            const Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
            if (inst.active)
                n.remove_material_override();
            else {
                Render::Material material;
                switch (tag->type)
                {
                case BedElementType::PlateDefault:  { material = BedMaterials::plate_default_override_material(m_device); break; }
                case BedElementType::PlateTextured: { material = BedMaterials::plate_textured_override_material(m_device, cc->bed()); break; }
                case BedElementType::Contour:       { material = BedMaterials::contour_override_material(m_device); break; }
                case BedElementType::Grid:          { material = BedMaterials::grid_override_material(m_device); break; }
                case BedElementType::PrintVolume:   { material = BedMaterials::print_volume_override_material(m_device); break; }
                case BedElementType::Model:         { material = BedMaterials::model_override_material(m_device); break; }
                default:                            { DEBUG_ASSERT(false); break; }
                }
                n.set_material_override(material);
            }
        }
    }, true);
}

void BedRenderUpdater::update_shadows(const Scene::Camera& cam)
{
    bool cam_pointing_upward = cam.pointing_upward();

    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Model ||
                tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::PlateTextured) {

                DEBUG_ASSERT(n.has_render_component());

                Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                const Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                if (!cam_pointing_upward && inst.active) {
                    if (tag->type == BedElementType::Model && m_scene_provider.scene().bed_model_cast_shadow())
                        n.render_component()->set_shadows(Render::Shadows{ true, true });
                    else
                        n.render_component()->set_shadows(Render::Shadows{ false, true });
                }
                else
                    n.render_component()->set_shadows(Render::Shadows{ false, false });
            }
        }
    }, true);
}

void BedRenderUpdater::update_positions()
{
    Scene::visit(m_scene_provider.scene().root(), [this](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Undefined) {
                DEBUG_ASSERT(m_project != nullptr);
                Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                n.set_world_transform(inst.matrix().matrix());
            }
        }
    }, true);
}

void BedRenderUpdater::update_elements_state()
{
    Scene::visit(m_scene_provider.scene().root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Contour || tag->type == BedElementType::PrintVolume) {
                DEBUG_ASSERT(m_project != nullptr);
                Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                if (tag->type == BedElementType::Contour)
                    n.set_enabled(inst.contour_enabled);
                else if (tag->type == BedElementType::PrintVolume)
                    n.set_enabled(inst.print_volume_enabled);
            }
        }
    }, true);
}

void BedRenderUpdater::camera_updated(const Scene::Camera& cam)
{
    bool cam_pointing_upward = cam.pointing_upward();
    auto& scene = m_scene_provider.scene();
    Scene::visit(scene.root(), [&](Scene::Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            // turn beds' plate and model visibility on/off in dependence of camera position/orientation
            if (tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::Model)
                n.set_enabled(!cam_pointing_upward);
            else if (tag->type == BedElementType::PlateTextured) {
                // change material in dependence of camera position/orientation
                DEBUG_ASSERT(m_project != nullptr);
                Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance& inst = cc->find_bed_instance(tag->instance_id);
                if (inst.active) {
                    if (cam_pointing_upward)
                        n.set_material_override(BedMaterials::plate_textured_override_material(m_device, cc->bed()));
                    else
                        n.remove_material_override();
                }
            }
        }
    }, true);

    update_shadows(cam);
}

void BedRenderUpdater::on_selected_project_changed(size_t index)
{
    m_project = const_cast<Domain::Project*>(&m_workbench.project(index));
}

} // namespace Slic3r::App::Plater
