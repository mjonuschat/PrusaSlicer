#include "Slic3r/App/Plater/Scoped3mfThumbnailSceneCustomizer.hpp"

namespace Slic3r::App::Plater {

Scoped3mfThumbnailSceneCustomizer::Scoped3mfThumbnailSceneCustomizer(Scene::Scene& scene, const Domain::Project& project,
    Scene::CameraProjectionType camera_type)
    : ScopedThumbnailSceneCustomizerBase(scene, project, camera_type)
{
    // store values that are going to be changed
    store_shading_type();
    store_background_enabled();
    store_shadows_aabb();

    // hide geometry
    hide_gizmos();
    hide_selection_aabb();
    hide_non_part_volumes();
    hide_bed_accessories();

    // set materials
    disable_bed_override_material();
    disable_volumes_override_material();
    override_non_printable_volumes_material();
    set_shadows();

    // set aabb for shadows
    Eigen::AlignedBox3d world_aabb = volume_parts_aabb();
    set_shadows_aabb(world_aabb);

    // setup scene
    set_background_enabled(false);
    set_shading_type(Scene::ShadingType::PBR);

    set_camera_trackball(world_aabb);
    zoom_to_box(world_aabb);
    update_camera_frustum();
}

} // namespace Slic3r::App::Plater
