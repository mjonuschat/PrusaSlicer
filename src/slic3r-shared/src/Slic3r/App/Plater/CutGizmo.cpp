///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/CutGizmo.hpp"
#include "Slic3r/App/Plater/CutDialog.hpp"
#include "Slic3r/App/Plater/CutUtils.hpp"
#include "Slic3r/Domain/CutConnector.hpp"

//#include "Slic3r/App/Plater/SceneBuilder.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener

#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/enum_bitmask.hpp"

#include "libslic3r/Geometry.hpp"

using namespace Slic3r::App::Yoga;


static const ImColor UPPER_PART_COLOR = ImVec4{ 0.0f, 1.0f, 1.0f, 1.0f };
static const ImColor LOWER_PART_COLOR = ImVec4{ 1.0f, 0.0f, 1.0f, 1.0f };

namespace Slic3r::App::Plater {

using namespace Slic3r::Domain;
using namespace Slic3r::Biz;

CutGizmo::CutGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor& project_interactor
) :
    m_device(device),
    m_scene_presenter(scene_presenter),
    m_project_interactor(project_interactor)
{
    m_dialog = std::make_unique<CutDialog>();

    m_dialog->callbacks().keep_as_part_changed = [this](bool keep_as_part) {
        m_keep_as_parts = keep_as_part;
    };
}

void CutGizmo::on_activated()
{
    m_dialog->set_current_connetor_shape(Domain::CutConnectorShape::Circle);
    m_dialog->set_current_connetor_style(Domain::CutConnectorStyle::Prism);
    m_dialog->set_current_connetor_type(Domain::CutConnectorType::Snap);
}

void CutGizmo::on_deactivated() {}

Scene::ToolType CutGizmo::type() const
{
    return Scene::ToolType::CutGizmo;
}

Yoga::GizmoDialog* CutGizmo::ui_dialog()
{
    return m_dialog.get();
}

Scene::GizmoActivationState CutGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState();
}

// code is borrowed from:
// #include <arrange-wrapper/SceneBuilder.hpp>

Domain::BoundingBox3d instance_bounding_box(const Domain::ModelInstance& mi,
    const Domain::Transform3d& tr = Domain::Transform3d::Identity(),
    bool dont_translate = false)
{
    using Slic3r::Biz::Algorithms::BoundingBox::merge;

    Domain::BoundingBox3d bb;
    const Domain::Transform3d inst_matrix
        = dont_translate ? mi.get_transformation().get_matrix_no_offset()
        : mi.get_transformation().get_matrix();

    for (Domain::ModelVolume* v : mi.get_object()->volumes) {
        if (v->is_model_part()) {
            bb = merge(bb, Slic3r::Biz::Algorithms::ModelVolume::transformed_bounding_box(*v, tr * inst_matrix * v->get_matrix()));
        }
    }

    return bb;
}

void CutGizmo::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{
    if (selection.empty() || selection.mode != Slic3r::Biz::Scene::SelectionMode::Instance) {
        // on_deactivated();

        // We can’t perform a cut for multiple objects simultaneously.
        return;
    }

    const Domain::Project& project = m_project_interactor.selected_project();

    for (const Domain::ElementRef& element : selection.elements) {
        assert(element.volume_id == 0); // is object
        const Domain::ModelObject* object = project.find_object_by_id(element.object_id);
        const Domain::ModelInstance* mi = project.find_instance_by_id(element.object_id, element.instance_id);
        Domain::BoundingBox3d bbox = instance_bounding_box(*mi);

        Domain::Vec3d bb_size = bbox.max - bbox.min;
        m_dialog->set_build_size(bb_size);

        m_plane_center = bbox.min + 0.5 * bb_size;
    }
}

Domain::Transform3d CutGizmo::get_cut_matrix()
{
    if (!m_selected_instance)
        return Domain::Transform3d::Identity();

    // m_cut_z is the distance from the bed. Subtract possible SLA elevation.
    const double sla_shift_z = 0.;// selection.get_first_volume()->get_sla_shift_z();

    const Domain::Vec3d instance_offset = m_selected_instance->get_offset();
    Domain::Vec3d cut_center_offset = m_plane_center - instance_offset;
    cut_center_offset.z() -= sla_shift_z;

    return Domain::translation_transform(cut_center_offset) * m_rotation_m;
}

bool CutGizmo::can_perform_cut() const
{
    return true;
}


void CutGizmo::apply_connectors_in_model(ModelObject* mo, int& dowels_count)
{
    if (m_dialog->is_planar_mode()) {
//        clear_selection();

        for (CutConnector& connector : mo->cut_connectors) {
            connector.rotation_m = m_rotation_m;

            if (connector.attribs.type == CutConnectorType::Dowel) {
                if (connector.attribs.style == CutConnectorStyle::Prism)
                    connector.height *= 2;
                dowels_count++;
            }
            else {
                // calculate shift of the connector center regarding to the position on the cut plane
                connector.pos += m_cut_normal * 0.5 * double(connector.height);
            }
        }
        apply_cut_connectors(mo, _u8L("Connector"));
    }
}

static indexed_triangle_set get_connector_mesh(CutConnectorAttributes connector_attributes)
{
    indexed_triangle_set connector_mesh;

    int   sectorCount{ 1 };
    switch (CutConnectorShape(connector_attributes.shape)) {
    case CutConnectorShape::Triangle:
        sectorCount = 3;
        break;
    case CutConnectorShape::Square:
        sectorCount = 4;
        break;
    case CutConnectorShape::Circle:
        sectorCount = 360;
        break;
    case CutConnectorShape::Hexagon:
        sectorCount = 6;
        break;
    default:
        break;
    }

    // define those values
    double m_snap_space_proportion, m_snap_bulge_proportion;
    double PI = 3.14;

    if (connector_attributes.type == CutConnectorType::Snap)
        connector_mesh = Biz::Algorithms::TriangleMesh::its_make_snap(1.0, 1.0, m_snap_space_proportion, m_snap_bulge_proportion);
    else if (connector_attributes.style == CutConnectorStyle::Prism)
        connector_mesh = Biz::Algorithms::TriangleMesh::its_make_cylinder(1.0, 1.0, (2 * PI / sectorCount));
    else if (connector_attributes.type == CutConnectorType::Plug)
        connector_mesh = Biz::Algorithms::TriangleMesh::its_make_frustum(1.0, 1.0, (2 * PI / sectorCount));
    else
        connector_mesh = Biz::Algorithms::TriangleMesh::its_make_frustum_dowel(1.0, 1.0, sectorCount);

    return connector_mesh;
}

void CutGizmo::apply_cut_connectors(ModelObject* mo, const std::string& connector_name)
{
    if (mo->cut_connectors.empty())
        return;

    using namespace Geometry;

    size_t connector_id = mo->cut_id.connectors_cnt();
    for (const CutConnector& connector : mo->cut_connectors) {
        TriangleMesh mesh = TriangleMesh(get_connector_mesh(connector.attribs));
        ModelVolume* new_volume = Biz::Algorithms::ModelObject::add_volume(mo, std::move(mesh), ModelVolumeType::NEGATIVE_VOLUME);

        // Transform the new modifier to be aligned inside the instance
        new_volume->set_transformation(translation_transform(connector.pos) * connector.rotation_m *
            rotation_transform(-connector.z_angle * Vec3d::UnitZ()) *
            scale_transform(Vec3f(connector.radius, connector.radius, connector.height).cast<double>()));

        new_volume->cut_info = { connector.attribs.type, connector.radius_tolerance, connector.height_tolerance };
        new_volume->name = connector_name + "-" + std::to_string(++connector_id);
    }
    mo->cut_id.increase_connectors_cnt(mo->cut_connectors.size());

    // delete all connectors
    mo->cut_connectors.clear();
}

static void update_object_cut_id(CutId& cut_id, ModelObjectCutAttributes attributes, const int dowels_count)
{
    // we don't save cut information, if result will not contains all parts of initial object
    if (!attributes.keep_upper ||
        !attributes.keep_lower ||
        attributes.invalidate_cut_info)
        return;

    if (!cut_id.valid())
        cut_id.init();
    // increase check sum, if it's needed
    {
        int cut_obj_cnt = -1;
        if (attributes.keep_upper)    cut_obj_cnt++;
        if (attributes.keep_lower)    cut_obj_cnt++;
        if (attributes.create_dowels) cut_obj_cnt += dowels_count;
        if (cut_obj_cnt > 0)
            cut_id.increase_check_sum(size_t(cut_obj_cnt));
    }
}

void CutGizmo::perform_cut()
{
    if (!can_perform_cut())
        return;

    Domain::ModelObject* mo = m_selected_object;
    if (!mo)
        return;

    // deactivate CutGizmo and than perform a cut
//!    m_parent.reset_all_gizmos();

    // perform cut
    {
//        Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Cut by Plane"));

        // This shall delete the part selection class and deallocate the memory.
        ScopeGuard part_selection_killer([this]() { m_part_selection = PartSelection(); });

        const bool cut_with_groove = !m_dialog->is_planar_mode();
        const bool cut_by_contour = !cut_with_groove && m_part_selection.valid();

        Domain::ModelObject* cut_mo = cut_by_contour ? m_part_selection.model_object() : nullptr;
        if (cut_mo)
            cut_mo->cut_connectors = mo->cut_connectors;
        else
            cut_mo = mo;

        int dowels_count = 0;
        const bool has_connectors = !mo->cut_connectors.empty();
        // update connectors pos as offset of its center before cut performing
 //       apply_connectors_in_model(cut_mo , dowels_count);

 //       ys_FIXME:: set wxBusyCursor ;

        ModelObjectCutAttributes attributes = {
            .keep_upper = has_connectors ? true : m_keep_upper,
            .keep_lower = has_connectors ? true : m_keep_lower,
            .keep_as_parts = has_connectors ? false : m_keep_as_parts,
            .flip_upper = m_flip_upper,
            .flip_lower = m_flip_lower,
            .place_on_cut_upper = m_place_on_cut_upper,
            .place_on_cut_lower = m_place_on_cut_lower,
            .create_dowels = dowels_count > 0,
            .invalidate_cut_info = !has_connectors && !cut_with_groove && !cut_mo->cut_id.valid() 
        };

        // update cut_id for the cut object in respect to the attributes
        update_object_cut_id(cut_mo->cut_id, attributes, dowels_count);

        //get instance index
        size_t instance_idx = 0;
        for (const auto* inst : mo->instances) {
            if (inst == m_selected_instance)
                break;
            instance_idx++;
        }
        ASSERT(instance_idx < mo->instances.size());

        Cut cut(cut_mo, instance_idx, get_cut_matrix(), attributes);
        const ModelObjectPtrs& new_objects = //cut_by_contour    ? cut.perform_by_contour(m_part_selection.get_cut_parts(), dowels_count):
                                             //cut_with_groove   ? cut.perform_with_groove(m_groove, m_rotation_m) :
                                                                 cut.perform_with_plane();
/*
        check_objects_after_cut(new_objects);

        // save cut_id to post update synchronization
        const CutId cut_id = cut_mo->cut_id;

        // update cut results on plater and in the model 
        plater->apply_cut_object_to_model(object_idx, new_objects);

        synchronize_model_after_cut(plater->model(), cut_id);
*/    }
}

} // namespace Slic3r::App::Plater
