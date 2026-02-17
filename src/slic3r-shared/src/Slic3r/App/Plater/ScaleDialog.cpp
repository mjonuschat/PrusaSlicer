#include "Slic3r/App/Plater/ScaleDialog.hpp"
#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Plater/TripleInput.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/App/Plater/PlaterGizmosHelper.hpp"
#include "Slic3r/Math.hpp"

namespace Slic3r::App::Plater {

using Biz::_u8L;
using Yoga::Orientation;
using Yoga::Text;
using Domain::BoundingBox3d;
using Domain::SquareMatrix4d;
using Domain::SquareMatrix3d;
using Domain::Vec3d;

void reset_scale() {
}

ScaleDialog::ScaleDialog(
    App::Plater::PlaterScenePresenter& scene_provider,
    Biz::ProjectInteractor& project_interactor
) :
    Yoga::GizmoWindow{_u8L("Scale"), Render::Icon::Scale},
    m_scene_provider{scene_provider},
    m_project_interactor{project_interactor},
    m_projects{project_interactor}
{
    m_project_interactor.scene_interactor()
        .add_listener<Biz::Scene::ISceneSelectionChangedListener>(this);
    m_project_interactor.add_listener<Biz::ISelectedProjectChangedListener>(this);
    m_scene_provider.add_listener<App::Plater::ISelectionBoundingBoxChangedListener>(this);

    content()->set_padding({20, 20});
    content()->set_orientation(Orientation::Vertical);

    const double gap{10.0};
    content()->set_gap(gap);

    auto revert_row{content()->emplace_back<Yoga::Item>()};
    revert_row->set_justify_content(YGJustifyFlexEnd);
    revert_row->set_height(0);
    m_revert_button = revert_row->emplace_back<Yoga::LayoutButton>(
        "",
        Render::Icon::RevertButton,
        "Revert scale"
    );
    m_revert_button->set_min_size({25.0, 25.0});
    m_revert_button->callbacks().action = [this]()
    {
        ProjectContext& project_context{m_projects.selected()};
        if (project_context.reset_scale_candidates.empty()) {
            return;
        }
        const bool was_floating{m_place_on_bed_button->is_floating};
        m_project_interactor.scene_interactor().set_element_transforms(
            project_context.reset_scale_candidates
        );
        if (!was_floating) {
            m_place_on_bed_button->trigger();
        }
    };

    auto title = content()->emplace_back<Text>("Size");
    title->set_font_type(Render::ImguiFontType::Bold);

    m_absolute_input = content()->emplace_back<TripleInput>(_u8L("mm"));

    m_absolute_input->on_change = [this](const Domain::Vec3d& value, int index) {
        const std::optional<Vec3d> current_dimensions{get_current_absolute_scale()};
        if (!current_dimensions) {
            return;
        }

        const Vec3d scale_by{
            m_lock->checked() ? value(index) / (*current_dimensions)(index) *Vec3d::Ones() :
                                value.cwiseQuotient(*current_dimensions).eval()
        };
        apply_relative_scale(scale_by);
        reload();
    };

    m_absolute_percent_input_item = content()->emplace_back<Yoga::Item>();
    m_absolute_percent_input_item->set_orientation(Orientation::Vertical);
    m_absolute_percent_input_item->set_gap(gap);
    // I wanted to align-items: flex-end; to align the button. Does not work.
    auto* header_item{m_absolute_percent_input_item->emplace_back<Yoga::Item>()};
    auto* percentage_text{header_item->emplace_back<Yoga::Text>(_u8L("Scale"))};
    percentage_text->set_font_type(Render::ImguiFontType::Bold);
    m_absolute_percent_input = m_absolute_percent_input_item->emplace_back<TripleInput>(_u8L("%"));
    m_absolute_percent_input->on_change = [this](const Domain::Vec3d& value, int index)
    {
        const std::optional<Vec3d> current_scale{get_current_relative_scale()};
        if (!current_scale) {
            return;
        }
        const Vec3d scale_by{
            m_lock->checked() ? value(index) / 100.0 / (*current_scale)(index) *Vec3d::Ones() :
                                (value / 100.0).cwiseQuotient(*current_scale).eval()
        };
        apply_relative_scale(scale_by);
        reload();
    };

    m_relative_input_item = content()->emplace_back<Yoga::Item>();
    m_relative_input_item->set_orientation(Orientation::Vertical);
    m_relative_input_item->set_gap(gap);
    auto* unaligned_text{m_relative_input_item->emplace_back<Yoga::Text>(_u8L("Relative scale"))};
    unaligned_text->set_font_type(Render::ImguiFontType::Bold);
    m_relative_input = m_relative_input_item->emplace_back<TripleInput>(_u8L("%"));
    m_relative_input->on_change = [this](const Domain::Vec3d& value, int index)
    {
        if (m_lock->checked()) {
        }
        const Vec3d scale_by{
            m_lock->checked() ? value(index) / 100.0 * Vec3d::Ones() : (value / 100.0).eval()
        };
        apply_relative_scale(scale_by);
    };

    m_place_on_bed_button = content()->emplace_back<PlaceOnBedButton>(
        m_scene_provider,
        m_project_interactor
    );

    m_reference_frame_picker = content()->emplace_back<ReferenceFramePicker>(m_project_interactor);

    m_lock = content()->emplace_back<Yoga::ToggleButton>("Uniform scale");
}

ScaleDialog::~ScaleDialog()
{
    m_project_interactor.scene_interactor()
        .remove_listener<Biz::Scene::ISceneSelectionChangedListener>(this);
    m_project_interactor.remove_listener<Biz::ISelectedProjectChangedListener>(this);
    m_scene_provider.remove_listener<App::Plater::ISelectionBoundingBoxChangedListener>(this);
}

void ScaleDialog::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection&
) {
    reload(project_id);
}

void ScaleDialog::on_scene_selection_transformed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection&
) {
    reload(project_id);
}

void ScaleDialog::on_scene_selection_bounding_box_changed(
    Domain::SelectionId project_id,
    const std::optional<Scene::OrientedBoundingBox>&
)
{
    reload(project_id);
}

void ScaleDialog::on_selected_project_changed_final(size_t index) {
    reload(index);
}

void ScaleDialog::on_activated(Domain::SelectionId project_id) {
    m_projects.selected().activated = true;
    m_reference_frame_picker->on_activated();
    reload(project_id);
}

void ScaleDialog::on_deactivated() {
    m_reference_frame_picker->on_deactivated();
    m_projects.selected().activated = false;
}

PlaceOnBedButton& ScaleDialog::place_on_bed_button() {
    return *m_place_on_bed_button;
}

std::optional<Domain::Vec3d> ScaleDialog::get_current_absolute_scale() const
{
    const std::optional<Scene::OrientedBoundingBox>& rotated_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!rotated_box) {
        return std::nullopt;
    }
    return rotated_box->dimensions;
}

static Domain::Vec3d get_relative_scale(const Domain::SquareMatrix3d& matrix) {
    const Eigen::JacobiSVD<Domain::SquareMatrix3d> svd(
        matrix,
        Eigen::ComputeFullU | Eigen::ComputeFullV
    );

    return (svd.matrixV() * svd.singularValues().asDiagonal() * svd.matrixV().transpose()).diagonal();
}

static bool same_euler_angels(
    const Domain::SquareMatrix3d& a,
    const Domain::SquareMatrix3d& b,
    const double max_angle_diff
)
{
    const Domain::Vec3d a_angles{a.eulerAngles(0, 1, 2)};
    const Domain::Vec3d b_angles{b.eulerAngles(0, 1, 2)};

    return ((a_angles - b_angles).cwiseAbs().array() < (max_angle_diff * Vec3d::Ones()).array())
        .all();
}

static std::optional<Domain::Vec3d> get_volume_relative_scale(
    const Domain::ModelInstance& instance,
    const Domain::ModelVolume& volume,
    const Scene::OrientedBoundingBox& obb
)
{
    const double max_angle_diff{Slic3r::deg2rad(0.001)};

    if (!same_euler_angels(
            (instance.get_matrix() * volume.get_matrix()).rotation(),
            obb.rotation,
            max_angle_diff
        ))
    {
        return std::nullopt;
    }

     return get_relative_scale(volume.get_matrix().matrix().block(0, 0, 3, 3));
}

std::optional<Domain::Vec3d> ScaleDialog::get_current_relative_scale() const
{
    const std::optional<Scene::OrientedBoundingBox>& rotated_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!rotated_box) {
        return std::nullopt;
    }
    const Biz::Scene::SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    const Domain::Workbench& workbench{m_project_interactor.workbench()};
    const Domain::Project& project{workbench.project(m_project_interactor.selected_project_id())};

    std::optional<Domain::Vec3d> first_volume_scale;
    for (const Domain::ElementRef& element : scene_interactor.object_selection().elements) {
        if (!element.has_instance()) {
            continue;
        }
        const Domain::ModelInstance* instance{project.find_instance_by_id(element.object_id, element.instance_id)};
        if (element.has_volume()) {
            const Domain::ModelVolume* volume{project.find_volume_by_id(element.object_id, element.volume_id)};
            const std::optional<Domain::Vec3d> scale{get_volume_relative_scale(*instance, *volume, *rotated_box)};
            if (!scale) {
                return std::nullopt;
            }
            if (!first_volume_scale) {
                first_volume_scale = *scale;
            } else {
                if(!first_volume_scale->isApprox(*scale)) {
                    return std::nullopt;
                }
            }
        } else {
            for (const Domain::ModelVolume* volume : instance->get_object()->volumes) {
                const std::optional<Domain::Vec3d> scale{get_volume_relative_scale(*instance, *volume, *rotated_box)};
                if (!scale) {
                    return std::nullopt;
                }
                if (!first_volume_scale) {
                    first_volume_scale = *scale;
                } else {
                    if(!first_volume_scale->isApprox(*scale)) {
                        return std::nullopt;
                    }
                }
            }
        }
    }

    return first_volume_scale;
}

void ScaleDialog::reload(std::optional<Domain::SelectionId> project_id)
{
    ProjectContext& project_context{m_projects.selected()};
    if (!project_context.activated) {
        return;
    }
    if (project_id && project_id != m_project_interactor.selected_project_id()) {
        return;
    }

    std::optional<Vec3d> absolute_scale{get_current_absolute_scale()};
    if (!absolute_scale) {
        return;
    }
    m_absolute_input->set_value(*absolute_scale);

    m_absolute_percent_input_item->set_visible(false);
    m_relative_input_item->set_visible(false);

    const std::optional<Vec3d> relative_scale{get_current_relative_scale()};
    if (relative_scale) {
        m_revert_button->set_visible(!relative_scale->isApprox(Domain::Vec3d{1.0, 1.0, 1.0}));
        m_absolute_percent_input->set_value((*relative_scale) * 100);
        m_absolute_percent_input_item->set_visible(true);
    } else {
        m_relative_input->set_value({100, 100, 100});
        m_relative_input_item->set_visible(true);
    }

    project_context.reset_scale_candidates = get_reset_scale_candidates();

    if (project_context.reset_scale_candidates.empty()) {
        m_revert_button->set_visible(false);
    } else {
        m_revert_button->set_visible(true);
    }
}

void ScaleDialog::apply_relative_scale(const Domain::Vec3d& scale_by)
{
    if ((scale_by.array() <= 0).any()) {
        return;
    }

    const std::optional<Scene::OrientedBoundingBox>& bounding_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!bounding_box) {
        return;
    }

    const double upper_bound{std::sqrt(3) * bounding_box->dimensions.maxCoeff()};
    const double new_upper_bound{upper_bound * scale_by.maxCoeff()};
    if (new_upper_bound > upper_bound && new_upper_bound > 1e6) {
        return;
    }

    Biz::Scene::TransformMemento memento;
    memento.forced_volume_mode = true;
    Biz::Scene::SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};

    const Domain::SquareMatrix4d transormation{
        get_scale_matrix(bounding_box->rotation, bounding_box->center, scale_by)
    };

    if (std::abs(transormation.determinant()) < 1e-6) {
        return;
    }

    const bool was_on_bed{!m_place_on_bed_button->is_floating};

    scene_interactor.transform_selection(
        get_scale_matrix(
            bounding_box->rotation,
            bounding_box->center,
            scale_by
        ),
        memento
    );
    scene_interactor.finalize_transform_selection(memento, false);

    if (was_on_bed) {
        m_place_on_bed_button->trigger();
    };
}

Domain::SquareMatrix4d remove_scale(
    const Domain::Transform3d& volume_trafo,
    const Domain::Transform3d& instance_trafo,
    const Domain::Vec3d& center
)
{
    const Domain::SquareMatrix3d rotation{volume_trafo.rotation()};

    const Domain::Vec3d center_in_volume_coords{
        volume_trafo.inverse() * instance_trafo.inverse() * center
    };
    const Domain::Vec3d center_in_instance_coords{instance_trafo.inverse() * center};

    Domain::SquareMatrix4d result{Domain::SquareMatrix4d::Identity()};
    result.block(0, 0, 3, 3) = rotation;
    result.col(3).head<3>()  = center_in_instance_coords - rotation * center_in_volume_coords;

    return result;
}

Biz::Scene::SceneInteractor::ElementTransforms ScaleDialog::get_reset_scale_candidates() const
{
    Biz::Scene::SceneInteractor& scene_interactor{m_project_interactor.scene_interactor()};
    Biz::Scene::SceneInteractor::ElementTransforms result;

    std::optional<Scene::OrientedBoundingBox> bounding_box{
        m_scene_provider.selection_bounding_box()
    };
    if (!bounding_box) {
        return {};
    }

    for (const Domain::ElementRef& element : scene_interactor.object_selection().elements) {
        if (!element.has_instance()) {
            continue;
        }
        const Domain::ModelInstance* instance{
            m_project_interactor.workbench()
                .project(m_project_interactor.selected_project_id())
                .find_instance_by_id(element.object_id, element.instance_id)
        };
        const Domain::Transform3d instance_matrix{instance->get_matrix()};

        if (element.has_volume()) {
            const Domain::ElementRef ref{element.object_id, 0, element.volume_id};
            const Domain::ModelVolume* volume{
                m_project_interactor.workbench()
                    .project(m_project_interactor.selected_project_id())
                    .find_volume_by_id(element.object_id, element.volume_id)
            };
            const Domain::Transform3d volume_matrix{volume->get_matrix()};
            const SquareMatrix4d no_scale{remove_scale(volume_matrix, instance_matrix, bounding_box->center)};
            if (!volume_matrix.matrix().isApprox(no_scale)) {
                result.insert_or_assign(ref, no_scale);
            }
        } else {

            for (const Domain::ModelVolume* volume : instance->get_object()->volumes) {
                const Domain::ElementRef ref{element.object_id, 0, volume->id().id};
                const Domain::Transform3d volume_matrix{volume->get_matrix()};
                const SquareMatrix4d no_scale{remove_scale(volume_matrix, instance_matrix, bounding_box->center)};
                if (!volume_matrix.matrix().isApprox(no_scale)) {
                    result.insert_or_assign(ref, no_scale);
                }
            }
        }
    }

    return result;
}
} // namespace Slic3r::App::Plater
