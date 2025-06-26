#include "Slic3r/App/Plater/ScopedBedThumbnailSceneCustomizer.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/BedRef.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/App/Scene/CameraHelper.hpp"

namespace Slic3r::App::Plater {

ScopedBedThumbnailSceneCustomizer::ScopedBedThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::BedRef& bed_ref,
    const Domain::BedInstance& bed_instance, Scene::CameraProjectionType camera_type)
  : m_scene(scene)
{
    //
    // store values that are going to be changed
    //

    // shading
    m_cache.shadows_enabled = m_scene.shadows_enabled();
    m_cache.ao_enabled = m_scene.ao_enabled();
    m_cache.pbr_enabled = m_scene.pbr_enabled();

    // camera
    Scene::Camera& camera = m_scene.camera();
    m_cache.camera_model = camera.model();
    m_cache.camera_zoom = camera.zoom();
    m_cache.switch_camera_projection_type = camera_type != camera.cam_projection().type();

    // camera trackball
    Scene::CameraTrackballController& trackball = m_scene.camera_trackball();
    m_cache.trackball_target = trackball.target();
    m_cache.trackball_pivot = trackball.pivot();
    m_cache.trackball_azimuth = trackball.azimuth();
    m_cache.trackball_zenith = trackball.zenith();
    m_cache.trackball_distance = trackball.distance_to_target();
    m_cache.trackball_view_rotation = trackball.view_rotation();

    // scene
    m_cache.shadows_aabb = m_scene.shadows_aabb();

    // hide gizmos
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        if (n.has_render_component() &&
            n.render_component()->layer_index() == int(PlaterSceneLayer::GizmoHandles)) {
            n.set_enabled(false);
            m_cache.hidden_nodes.push_back(&n);
        }
    });

    // hide volumes which do not belong to the bed instance
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        const auto* tag = n.tag_of_type<SceneNodeTag>();
        if (tag != nullptr) {
            auto it = std::find_if(bed_instance.model_instances.begin(), bed_instance.model_instances.end(),
                [&](Domain::ModelInstance* inst) { return inst->id().id == tag->instance_id; });
            if (it == bed_instance.model_instances.end()) {
                n.set_enabled(false);
                m_cache.hidden_nodes.push_back(&n);
            }
        }
    });

    // hide all non-part volumes which belong to the bed instance
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        const auto* tag = n.tag_of_type<SceneNodeTag>();
        if (tag != nullptr && tag->volume_id > 0 && tag->volume_type != Domain::ModelVolumeType::MODEL_PART) {
            n.set_enabled(false);
            m_cache.hidden_nodes.push_back(&n);
        }
    });

    // hide other beds
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
        if (tag != nullptr && tag->type == Scene::BedElementType::Undefined &&
            (tag->config_container_id != bed_ref.config_container_id || tag->instance_id != bed_ref.instance_id)) {
            n.set_enabled(false);
            m_cache.hidden_nodes.push_back(&n);
        }
    });

    // hide bed elements
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
        if (tag != nullptr) {
            if (tag->type == Scene::BedElementType::AxesMain ||
                tag->type == Scene::BedElementType::Contour ||
                tag->type == Scene::BedElementType::PrintVolume) {
                n.set_enabled(false);
                m_cache.hidden_nodes.push_back(&n);
            }
        }
    });

    // bed override material (for unselected beds)
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        Scene::BedNodeTag* tag = n.tag_of_type<Scene::BedNodeTag>();
        if (tag != nullptr && n.has_material_override()) {
            m_cache.materials.push_back(std::make_pair(&n, *n.material_override()));
            n.remove_material_override();
        }
    });

    // override volumes material (to hide selection)
    Scene::visit(m_scene.root(), [&](Scene::Node& n) {
        const auto* tag = n.tag_of_type<SceneNodeTag>();
        if (tag != nullptr && n.has_material_override()) {
            m_cache.materials.push_back(std::make_pair(&n, *n.material_override()));
            n.remove_material_override();
        }
    });

    // set aabb for shadows
    std::vector<Domain::Vec3f> print_volume = Biz::Scene::BedGeometry::print_volume(bed_instance.bed);
    Eigen::AlignedBox3d bed_aabb;
    Domain::Vec3d bed_inst_offset = bed_instance.transformation.get_offset();
    for (const auto& v : print_volume) {
        bed_aabb.extend(bed_inst_offset + v.cast<double>());
    }
    m_scene.set_shadows_aabb(bed_aabb);

    // setup shading
    m_scene.set_pbr_enabled(true);

    // camera type
    if (m_cache.switch_camera_projection_type)
        camera.switch_projection_type();

    // setup camera trackball
    Eigen::AlignedBox3d world_aabb;
    Scene::visit(m_scene.root(), [&](const Scene::Node& n) {
        if (n.has_raycast_component())
            world_aabb.extend(n.raycast_component()->world_bounding_box(n.world_transform()).cast<double>());
    });
    trackball.set_target(world_aabb.center());
    trackball.set_distance_to_target(world_aabb.diagonal().norm());
    trackball.set_azimuth_and_zenith(0.25 * std::numbers::pi, 0.75 * std::numbers::pi);

    // setup camera zoom
    Scene::zoom_to_box(camera, world_aabb);
}

ScopedBedThumbnailSceneCustomizer::~ScopedBedThumbnailSceneCustomizer()
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

    // shading
    if (!m_cache.shadows_enabled)
        m_scene.set_shadows_enabled(m_cache.shadows_enabled);
    else if (!m_cache.ao_enabled)
        m_scene.set_ao_enabled(m_cache.ao_enabled);
    else if (!m_cache.pbr_enabled)
        m_scene.set_pbr_enabled(m_cache.pbr_enabled);

    // shadows aabb
    m_scene.set_shadows_aabb(m_cache.shadows_aabb);

    // camera trackball
    Scene::CameraTrackballController& trackball = m_scene.camera_trackball();
    trackball.set_target(m_cache.trackball_target);
    trackball.set_pivot(m_cache.trackball_pivot);
    trackball.set_distance_to_target(m_cache.trackball_distance);
    trackball.set_azimuth_and_zenith(m_cache.trackball_azimuth, m_cache.trackball_zenith);
    trackball.set_view_rotation(m_cache.trackball_view_rotation);

    // camera
    Scene::Camera& camera = m_scene.camera();
    if (m_cache.switch_camera_projection_type)
        camera.switch_projection_type();

    camera.set_model(m_cache.camera_model);
    camera.set_zoom(m_cache.camera_zoom);
}

} // namespace Slic3r::App::Plater

