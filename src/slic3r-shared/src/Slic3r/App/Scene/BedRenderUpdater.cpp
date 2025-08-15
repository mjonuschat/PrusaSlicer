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

using Domain::BedRef;

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

            const BedRef bed_ref{cc->id().id, inst->id().id};
            if (m_scene_interactor.bed_selection().is_selected(bed_ref)) {
                n.remove_material_override();
                if (tag->type == BedElementType::Label && m_scene_interactor.bed_selection().last_selected_bed() != bed_ref) {
                    n.set_material_override(
                        BedMaterials::label_secondary_selection_material(m_device, inst->label())
                    );
                }
            } else {
                Render::Material material;
                switch (tag->type)
                {
                case BedElementType::PlateDefault:  { material = BedMaterials::plate_default_override_material(m_device); break; }
                case BedElementType::PlateTextured: { material = BedMaterials::plate_textured_override_material(n.render_component()->material()); break; }
                case BedElementType::Contour:       { material = BedMaterials::contour_override_material(m_device); break; }
                case BedElementType::Grid:          { material = BedMaterials::grid_override_material(m_device); break; }
                case BedElementType::PrintVolume:   { material = BedMaterials::print_volume_override_material(m_device); break; }
                case BedElementType::Model:         { material = BedMaterials::model_override_material(m_device); break; }
                case BedElementType::Label:         { material = BedMaterials::label_override_material(m_device, inst->label()); break; }
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

                const bool is_active{m_scene_interactor.bed_selection().is_selected(BedRef{cc->id().id, inst->id().id})};
                if (!cam_pointing_upward && is_active) {
                    if (tag->type == BedElementType::Model && Scene::Scene::graphics_settings().bed_model_cast_shadow())
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
    size_t bed_instances_count = 0;

    visit(
        m_scene_provider.scene().root(),
        [&](Node& n) {
            BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
            if (tag == nullptr) {
                return;
            }
            if (tag->type == BedElementType::Undefined) {
                ++bed_instances_count;
                return;
            }
            if (tag->type != BedElementType::Contour
                && tag->type != BedElementType::PrintVolume
                && tag->type != BedElementType::AxesMain)
            {
                return;
            }

            ASSERT(m_project != nullptr);
            const Domain::ConfigContainer* cc = m_project->find_config_container(
                tag->config_container_id
            );
            ASSERT(cc != nullptr);
            const Domain::BedInstance* inst = Domain::find_by_id(cc->bed_instances(), tag->instance_id);
            if (inst == nullptr) {
                return;
            }

            const BedRef bed_ref{cc->id().id, inst->id().id};
            const bool is_active{m_scene_interactor.bed_selection().is_selected(bed_ref)};
            // update elements' visibility
            switch (tag->type) {
            case BedElementType::Contour: {
                n.set_enabled(bed_ref == m_scene_interactor.bed_selection().last_selected_bed());
                break;
            }
            case BedElementType::PrintVolume: {
                n.set_enabled(inst->print_volume_enabled);
                break;
            }
            case BedElementType::AxesMain: {
                n.set_enabled(is_active);
                break;
            }
            default:
                break;
            }
        },
        true
    );

    visit(
        m_scene_provider.scene().root(),
        [&](Node& n) {
            BedNodeTag* tag = n.tag_of_type<BedNodeTag>();
            if (tag != nullptr && tag->type == BedElementType::Label) {
                n.set_enabled(bed_instances_count > 1);
            }
        },
        true
    );
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

                const bool is_active{m_scene_interactor.bed_selection().is_selected(BedRef{cc->id().id, inst->id().id})};
                if (is_active) {
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

                const bool is_active{m_scene_interactor.bed_selection().is_selected(BedRef{cc->id().id, inst->id().id})};
                if (is_active) {
                    Transform3d scale = Transform3d::Identity();
                    scale.scale(std::min(1.0, 1.0 / cam.zoom() * 10.0));
                    n.set_local_transform(scale.matrix());
                }
            }
        }
    }, true);

    update_shadows(cam);
}

void BedRenderUpdater::on_selected_project_changed(Domain::SelectionId project_id)
{
    m_project = &m_workbench.project(project_id);
    m_project_id = project_id;
}

} // namespace Slic3r::App::Scene
