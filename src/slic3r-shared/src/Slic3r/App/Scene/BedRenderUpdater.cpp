#include "Slic3r/App/Scene/BedRenderUpdater.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/BedMaterials.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Types.hpp"

#include "Slic3r/Assert.hpp"

using Slic3r::Domain::Transform3d;

namespace Slic3r::App::Scene {

void BedRenderUpdater::update_materials()
{
    visit(m_scene_provider.scene().root(), [&](Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr && tag->type != BedElementType::Undefined) {
            DEBUG_ASSERT(m_project != nullptr);
            const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
            DEBUG_ASSERT(cc != nullptr);
            const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
            if (inst == nullptr)
                return;

            if (inst->active)
                n.remove_material_override();
            else {
                Render::Material material;
                switch (tag->type)
                {
                case BedElementType::PlateDefault:  { material = BedMaterials::plate_default_override_material(m_device); break; }
                case BedElementType::PlateTextured: { material = BedMaterials::plate_textured_override_material(n.render_component()->material()); break; }
                case BedElementType::Contour:       { material = BedMaterials::contour_override_material(m_device); break; }
                case BedElementType::Grid:          { material = BedMaterials::grid_override_material(m_device); break; }
                case BedElementType::PrintVolume:   { material = BedMaterials::print_volume_override_material(m_device); break; }
                case BedElementType::Model:         { material = BedMaterials::model_override_material(m_device); break; }
                default:                            { break; }
                }
                n.set_material_override(material);
            }
        }
    }, true);
}

void BedRenderUpdater::update_shadows(const Camera& cam)
{
    bool cam_pointing_upward = cam.pointing_upward();

    visit(m_scene_provider.scene().root(), [&](Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Model ||
                tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::PlateTextured) {

                DEBUG_ASSERT(n.has_render_component());

                const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
                if (inst == nullptr)
                    return;
                if (!cam_pointing_upward && inst->active) {
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
    visit(m_scene_provider.scene().root(), [this](Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Undefined) {
                DEBUG_ASSERT(m_project != nullptr);
                const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
                if (inst == nullptr)
                    return;

                n.set_world_transform(inst->matrix().matrix());
            }
        }
    }, true);
}

void BedRenderUpdater::update_elements_state()
{
    visit(m_scene_provider.scene().root(), [&](Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == BedElementType::Contour ||
                tag->type == BedElementType::PrintVolume ||
                tag->type == BedElementType::AxesMain) {
                DEBUG_ASSERT(m_project != nullptr);
                const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
                if (inst == nullptr)
                    return;
                // update elements' visibility
                switch (tag->type) {
                case BedElementType::Contour:     { n.set_enabled(inst->contour_enabled); break; }
                case BedElementType::PrintVolume: { n.set_enabled(inst->print_volume_enabled); break; }
                case BedElementType::AxesMain:    { n.set_enabled(inst->active); break; }
                }
            }
        }
    }, true);
}

void BedRenderUpdater::camera_updated(const Camera& cam)
{
    bool cam_pointing_upward = cam.pointing_upward();
    auto& scene = m_scene_provider.scene();
    visit(scene.root(), [&](Node& n) {
        BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
        if (tag != nullptr) {
            // turn beds' plate and model visibility on/off in dependence of camera position/orientation
            if (tag->type == BedElementType::PlateDefault ||
                tag->type == BedElementType::Model)
                n.set_enabled(!cam_pointing_upward);
            else if (tag->type == BedElementType::PlateTextured) {
                // change material in dependence of camera position/orientation
                DEBUG_ASSERT(m_project != nullptr);
                const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
                if (inst == nullptr)
                    return;
                if (inst->active) {
                    if (cam_pointing_upward)
                        n.set_material_override(BedMaterials::plate_textured_override_material(n.render_component()->material()));
                    else
                        n.remove_material_override();
                }
            }
            else if (tag->type == BedElementType::AxesScaler) {
                // change axes scale in dependence of camera zoom
                DEBUG_ASSERT(m_project != nullptr);
                const Domain::ConfigContainer* cc = m_project->find_config_container(tag->config_container_id);
                DEBUG_ASSERT(cc != nullptr);
                const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
                if (inst == nullptr)
                    return;
                if (inst->active) {
                    Transform3d scale = Transform3d::Identity();
                    scale.scale(std::min(1.0, 1.0 / cam.zoom() * 10.0));
                    n.set_local_transform(scale.matrix());
                }
            }
        }
    }, true);

    update_shadows(cam);
}

void BedRenderUpdater::on_selected_project_changed(size_t index)
{
    m_project = &m_workbench.project(index);
}

} // namespace Slic3r::App::Scene
