#include "Slic3r/App/Plater/ScopedBedThumbnailSceneCustomizer.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/BedRef.hpp"

namespace Slic3r::App::Plater {

ScopedBedThumbnailSceneCustomizer::ScopedBedThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project,
    Domain::SelectionId bed_instance_id, Scene::CameraProjectionType camera_type)
    : ScopedThumbnailSceneCustomizerBase(scene, project, camera_type)
{
    const auto* cc = m_project.find_config_container_by_bed_instance_id(bed_instance_id);
    DEBUG_ASSERT(cc != nullptr);
    const Domain::BedInstance* bed_instance = m_project.find_bed_instance_by_id(bed_instance_id);
    DEBUG_ASSERT(bed_instance != nullptr);
    Domain::BedRef bed_ref{cc->id().id, bed_instance_id};

    // store values that are going to be changed
    store_shading_type();
    store_shadows_aabb();
    store_camera_synch_data();

    // hide geometry
    hide_gizmos();
    hide_selection_aabb();
    hide_volumes_outside_selected_bed_instances(*bed_instance);
    hide_non_part_volumes();
    hide_non_selected_bed_instances(bed_ref);
    hide_bed_accessories();

    // set materials
    disable_bed_override_material();
    disable_volumes_override_material();

    // set aabb for shadows
    set_shadows_aabb(bed_instance_aabb(*bed_instance));

    // setup scene
    set_shading_type(Scene::ShadingType::PBR);

    if (m_cache.camera_synch_data.type != uint8_t(m_camera_type))
        switch_camera_projection_type();

    Eigen::AlignedBox3d world_aabb = scene_aabb();
    set_camera_trackball(world_aabb);
    zoom_to_box(world_aabb);
    update_camera_frustum();
}

} // namespace Slic3r::App::Plater
