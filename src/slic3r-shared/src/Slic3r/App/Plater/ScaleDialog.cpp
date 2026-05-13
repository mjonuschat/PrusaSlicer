#include "Slic3r/App/Plater/ScaleDialog.hpp"
#include "Slic3r/App/Plater/ScaleWidget.hpp"
#include "Slic3r/App/ScaleHelpers.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App::Plater {

using Biz::_u8L;

ScaleDialog::ScaleDialog(Biz::ProjectInteractor& project_interactor) :
    GizmoWindow{_u8L("Scale"), Render::Icon::None, "S"}
{
    content()->set_padding({20_px, 20_px});
    content()->set_orientation(Yoga::Orientation::Vertical);
    content()->set_gap(20_px);

    auto reference_frame_picker{std::make_unique<ReferenceFramePicker>(
        project_interactor,
        Biz::Scene::SelectionReferenceFrame::Volume
    )};

    m_scale_widget = content()->emplace_back<ScaleWidget>(
        project_interactor,
        revert_button(),
        reference_frame_picker.get()
    );

    add_separator(content());
    content()->append(std::move(reference_frame_picker));
}

void ScaleDialog::on_activated(Domain::SelectionId project_id)
{
    m_scale_widget->on_activated(project_id);
}

void ScaleDialog::on_deactivated()
{
    m_scale_widget->on_deactivated();
}

} // namespace Slic3r::App::Plater
