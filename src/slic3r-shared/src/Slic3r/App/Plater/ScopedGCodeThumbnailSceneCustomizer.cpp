#include "Slic3r/App/Plater/ScopedGCodeThumbnailSceneCustomizer.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/CameraHelper.hpp"

namespace Slic3r::App::Plater {

ScopedGCodeThumbnailSceneCustomizer::ScopedGCodeThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project, Domain::SelectionId bed_instance_id, Scene::CameraProjectionType camera_type) :
    m_scene(scene)
{
    const auto* cc = project.find_config_container_by_bed_instance_id(bed_instance_id);
    DEBUG_ASSERT(cc != nullptr);
    const Domain::BedInstance* bed_instance = project.find_bed_instance_by_id(bed_instance_id);
    DEBUG_ASSERT(bed_instance != nullptr);
    Domain::BedRef bed_ref{cc->id().id, bed_instance_id};

    //
    // store values that are going to be changed
    //

    // shading
    m_cache.shading_type = Scene::Scene::graphics_settings().shading_type();

    // camera
    Scene::Camera& camera                       = m_scene.camera();
    Scene::CameraTrackballController& trackball = m_scene.camera_trackball();
    m_cache.camera_synch_data.model             = camera.model();
    m_cache.camera_synch_data.zoom              = camera.zoom();
    m_cache.camera_synch_data.type              = uint8_t(camera.cam_projection().type());
    m_cache.camera_synch_data.target            = trackball.target();
    m_cache.camera_synch_data.pivot             = trackball.pivot();
    m_cache.camera_synch_data.azimuth           = trackball.azimuth();
    m_cache.camera_synch_data.zenith            = trackball.zenith();
    m_cache.camera_synch_data.distance          = trackball.distance_to_target();
    m_cache.camera_synch_data.view_rotation     = trackball.view_rotation();

    // scene
    m_cache.shadows_aabb       = Scene::Scene::graphics_settings().shadows_aabb();
    m_cache.background_enabled = m_scene.background_enabled();

    // hide gizmos
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            if (n.has_render_component() && n.render_component()->layer_index() == int(PlaterSceneLayer::GizmoHandles))
            {
                n.set_enabled(false);
                m_cache.hidden_nodes.push_back(&n);
            }
        }
    );

    // hide all non selected bed instances
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
            if (tag != nullptr
                && tag->type == Scene::BedElementType::Undefined
                && (tag->config_container_id != bed_ref.config_container_id
                    || tag->instance_id != bed_ref.instance_id))
            {
                n.set_enabled(false);
                m_cache.hidden_nodes.push_back(&n);
            }
        }
    );

    // hide bed elements
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
            if (tag != nullptr) {
                if (tag->type == Scene::BedElementType::AxesMain
                    || tag->type == Scene::BedElementType::Contour
                    || tag->type == Scene::BedElementType::PrintVolume
                    || tag->type == Scene::BedElementType::Label)
                {
                    n.set_enabled(false);
                    m_cache.hidden_nodes.push_back(&n);
                }
            }
        }
    );

    // hide volumes which do not belong to the selected bed instance
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            const auto* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr) {
                auto it = std::find_if(
                    bed_instance->model_instances.begin(),
                    bed_instance->model_instances.end(),
                    [&](Domain::ModelInstance* inst) { return inst->id().id == tag->instance_id; }
                );
                if (it == bed_instance->model_instances.end()) {
                    n.set_enabled(false);
                    m_cache.hidden_nodes.push_back(&n);
                }
            }
        }
    );

    // hide non-printable volumes
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            const auto* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr) {
                const Domain::ModelInstance* model_inst = project.find_instance_by_id(tag->object_id, tag->instance_id);
                if (!model_inst->printable) {
                    n.set_enabled(false);
                    m_cache.hidden_nodes.push_back(&n);
                }
            }
        }
    );

    // hide modifier volumes
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            const auto* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr && tag->volume_id > 0 && tag->volume_type != Domain::ModelVolumeType::MODEL_PART)
            {
                n.set_enabled(false);
                m_cache.hidden_nodes.push_back(&n);
            }
        }
    );

    // bed override material (for unselected beds)
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
            if (tag != nullptr && n.has_material_override()) {
                m_cache.materials.push_back(std::make_pair(&n, *n.material_override()));
                n.remove_material_override();
            }
        }
    );

    // override volumes material (to hide selection)
    Scene::visit(
        m_scene.root(),
        [&](Scene::Node& n)
        {
            const auto* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr && n.has_material_override()) {
                m_cache.materials.push_back(std::make_pair(&n, *n.material_override()));
                n.remove_material_override();
            }
        }
    );

    // set aabb for shadows
    Eigen::AlignedBox3d world_aabb;
    Scene::visit(
        m_scene.root(),
        [&](const Scene::Node& n)
        {
            const auto* tag = n.tag_of_type<SceneNodeTag>();
            if (tag != nullptr && tag->volume_type == Domain::ModelVolumeType::MODEL_PART && n.has_raycast_component())
                world_aabb.extend(
                    n.raycast_component()->world_bounding_box(n.world_transform().matrix()).cast<double>()
                );
        }
    );
    Scene::Scene::set_shadows_aabb(world_aabb);

    // setup shading
    m_scene.set_background_enabled(false);
    Scene::Scene::set_shading_type(Scene::ShadingType::PBR);

    // camera type
    if (m_cache.camera_synch_data.type != uint8_t(camera_type))
        camera.switch_projection_type();

    // setup camera trackball
    trackball.set_target(world_aabb.center());
    trackball.set_distance_to_target(world_aabb.diagonal().norm());
    trackball.set_azimuth_and_zenith(0.25 * std::numbers::pi, 0.75 * std::numbers::pi);

    // setup camera zoom
    Scene::zoom_to_box(camera, world_aabb);
}

ScopedGCodeThumbnailSceneCustomizer::~ScopedGCodeThumbnailSceneCustomizer()
{
    //
    // restore values that were changed
    //

    // hidden nodes
    for (auto* n : m_cache.hidden_nodes) {
        n->set_enabled(true);
    }

    // override materials
    for (auto& [n, material] : m_cache.materials) {
        n->set_material_override(material);
    }

    m_scene.set_background_enabled(m_cache.background_enabled);

    // shading
    Scene::Scene::set_shading_type(m_cache.shading_type);

    // shadows aabb
    Scene::Scene::set_shadows_aabb(m_cache.shadows_aabb);

    // camera
    synchronize_camera(m_cache.camera_synch_data, m_scene.camera(), m_scene.camera_trackball());
}

} // namespace Slic3r::App::Plater
