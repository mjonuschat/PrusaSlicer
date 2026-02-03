#include "Slic3r/App/Plater/TranslationDialog.hpp"
#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/App/Plater/PlaterGizmosHelper.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/RadioButton.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

using Biz::_u8L;
using Biz::Scene::ObjectSelection;
using Biz::Scene::SceneInteractor;
using Domain::BoundingBox3d;
using Domain::ElementRef;
using Domain::ElementRefs;
using Domain::SquareMatrix4d;
using Domain::Vec3d;
using Yoga::ItemPtr;
using Yoga::Margins;
using Yoga::Orientation;
using Yoga::Text;

TranslationDialog::TranslationDialog(
    App::Plater::PlaterScenePresenter& scene_provider,
    Biz::ProjectInteractor& project_interactor
) :
    Yoga::GizmoWindow{_u8L("Translation"), Render::Icon::Move},
    m_scene_provider(scene_provider),
    m_project_interactor{project_interactor},
    m_projects{project_interactor}
{
    m_scene_provider.add_listener<App::Plater::ISelectionBoundingBoxChangedListener>(this);
    m_project_interactor.scene_interactor().add_listener<Biz::ISelectedBedInstancesChangedListener>(
        this
    );
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);

    content()->set_padding({20, 20});
    content()->set_orientation(Orientation::Vertical);
    content()->set_gap(10.0);

    m_absolute_input_row = content()->emplace_back<Yoga::Item>();
    m_absolute_input_row->set_orientation(Orientation::Vertical);
    auto absolute_text{m_absolute_input_row->emplace_back<Text>("Translation")};
    absolute_text->set_font_type(Render::ImguiFontType::Bold);
    m_absolute_input = m_absolute_input_row->emplace_back<TripleInput>(
        _u8L("mm"),
        get_axis_header({"X", "Y", "Z"})
    );
    m_absolute_input->on_change = [this](const Domain::Vec3d& value, int)
    {
        const std::optional<Scene::OrientedBoundingBox> bounding_box{
            m_scene_provider.selection_bounding_box()
        };

        const std::optional<Domain::Vec3d> reference_point{get_absolute_position_reference_point()};

        if (!bounding_box || !reference_point) {
            return;
        }

        const Domain::Vec3d current_value{bounding_box->center - *reference_point};
        apply_relative_translation(value - current_value);
    };

    m_relative_input_row = content()->emplace_back<Yoga::Item>();
    m_relative_input_row->set_orientation(Orientation::Vertical);
    auto relative_text{m_relative_input_row->emplace_back<Text>("Relative translation")};
    relative_text->set_font_type(Render::ImguiFontType::Bold);
    m_relative_input = m_relative_input_row->emplace_back<TripleInput>(
        _u8L("mm"),
        get_axis_header({"X", "Y", "Z"})
    );
    m_relative_input->on_change = [this](const Domain::Vec3d& value, int)
    { apply_relative_translation(value); };

    content()->emplace_back<PlaceOnBedButton>(
        m_scene_provider,
        m_project_interactor
    );

    m_reference_frame_picker = content()->emplace_back<ReferenceFramePicker>(m_project_interactor);
}

TranslationDialog::~TranslationDialog()
{
    m_scene_provider.remove_listener<App::Plater::ISelectionBoundingBoxChangedListener>(this);
    m_project_interactor.scene_interactor()
        .remove_listener<Biz::ISelectedBedInstancesChangedListener>(this);
    m_project_interactor.remove_listener<Biz::ISelectedProjectChangedListener>(this);
}

void TranslationDialog::on_selected_bed_instances_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::BedSelection&
)
{
    reload(project_id);
}

void TranslationDialog::on_scene_selection_bounding_box_changed(
    Domain::SelectionId project_id,
    const std::optional<Scene::OrientedBoundingBox>&
)
{
    reload(project_id);
}

void TranslationDialog::on_selected_project_changed_final(size_t index) {
    reload(index);
}

void TranslationDialog::on_activated()
{
    m_projects.selected().activated = true;
    m_reference_frame_picker->on_activated();
    reload(m_project_interactor.selected_project_id());
}

void TranslationDialog::on_deactivated()
{
    m_projects.selected().activated = false;
    m_reference_frame_picker->on_deactivated();
}

Domain::Vec3d get_selected_bed_translation(const Biz::ProjectInteractor& project_interactor)
{
    const Domain::Workbench& workbench{project_interactor.workbench()};
    const Domain::Project& project{workbench.project(project_interactor.selected_project_id())};
    const Biz::Scene::SceneInteractor& scene_interactor{project_interactor.scene_interactor()};
    const Domain::BedRef bed_ref{scene_interactor.bed_selection().last_selected_bed()};
    const Domain::BedInstance* bed_instance{project.find_bed_instance_by_id(bed_ref.instance_id)};
    return ASSERT_VAL(bed_instance)->transformation.get_matrix().translation();
}

std::optional<Vec3d> TranslationDialog::get_absolute_position_reference_point() const
{
    using Biz::Scene::SelectionReferenceFrame;
    using Biz::Scene::SelectionState;

    const SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    const Biz::Scene::ObjectSelection& selection{scene_interactor.object_selection()};
    const Biz::Scene::SelectionReferenceFrame reference_frame{
        scene_interactor.object_selection_reference_frame()
    };

    if (selection.empty()) {
        return std::nullopt;
    }
    const Domain::ElementRef& element{selection.elements.front()};
    ASSERT(element.has_instance());
    const Domain::ModelInstance* instance{
        m_project_interactor.workbench()
            .project(m_project_interactor.selected_project_id())
            .find_instance_by_id(element.object_id, element.instance_id)
    };

    const Domain::Vec3d bed_offset{get_selected_bed_translation(m_project_interactor)};

    if (selection.state() == SelectionState::SingleVolume) {
        ASSERT(selection.elements.size() == 1);
        ASSERT(element.has_volume());

        if (reference_frame == SelectionReferenceFrame::Bed) {
            return bed_offset;
        } else if (reference_frame == SelectionReferenceFrame::Instance) {
            return instance->get_matrix().translation();
        }
    }

    if (selection.state() == SelectionState::WholeInstance
        && reference_frame == SelectionReferenceFrame::Bed)
    {
        return bed_offset;
    }

    return std::nullopt;
}

void TranslationDialog::reload(Domain::SelectionId project_id)
{
    if (!m_projects.selected().activated) {
        return;
    }
    if (project_id != m_project_interactor.selected_project_id()) {
        return;
    }

    const std::optional<Scene::OrientedBoundingBox>& bounding_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!bounding_box) {
        return;
    }

    m_relative_input_row->set_visible(false);
    m_absolute_input_row->set_visible(false);

    const std::optional<Domain::Vec3d> reference_point{get_absolute_position_reference_point()};
    if (!reference_point) {
        m_relative_input_row->set_visible(true);
        m_relative_input->set_value({0, 0, 0});
    } else {
        m_absolute_input_row->set_visible(true);
        m_absolute_input->set_value(bounding_box->center - *reference_point);
    }
}

void TranslationDialog::apply_relative_translation(const Domain::Vec3d& translation)
{
    const std::optional<Scene::OrientedBoundingBox>& bounding_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!bounding_box) {
        return;
    }

    Biz::Scene::SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    scene_interactor.transform_selection(
        get_translation_matrix(bounding_box->rotation, translation)
    );
}

} // namespace Slic3r::App::Plater
